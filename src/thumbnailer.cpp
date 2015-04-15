/*
 * Copyright (C) 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Jussi Pakkanen <jussi.pakkanen@canonical.com>
 *              Michi Henning <michi.henning@canonical.com>
 */

#include <thumbnailer.h>

#include <internal/audioimageextractor.h>
#include <internal/gobj_memory.h>
#include <internal/image.h>
#include <internal/imagescaler.h>
#include <internal/lastfmdownloader.h>
#include <internal/make_directories.h>
#include <internal/ubuntuserverdownloader.h>
#include <internal/videoscreenshotter.h>

#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <core/persistent_string_cache.h>
#include <libexif/exif-loader.h>
#include <unity/util/ResourcePtr.h>

#include <fcntl.h>
#include <sys/stat.h>

using namespace std;
using namespace unity::thumbnailer::internal;

class ThumbnailerPrivate
{
private:
    string extract_exif_image(std::string const& filename);
    string extract_image_from_audio(string const& filename);
    string extract_image_from_video(string const& filename);
    string extract_image_from_other(string const& filename);
    string create_tmp_filename();
    string read_file(string const& filename);

public:
    AudioImageExtractor audio_;
    VideoScreenshotter video_;
    ImageScaler scaler_;
    core::PersistentStringCache::UPtr full_size_cache_;  // Small cache of full (original) size images.
    core::PersistentStringCache::UPtr thumbnail_cache_;  // Large cache of scaled images.
    std::unique_ptr<ArtDownloader> downloader_;

    ThumbnailerPrivate();

    string extract_image(string const& filename);
    string fetch_thumbnail(string const& key1,
                           string const& key2,
                           int desired_size,
                           function<string(string const&, string const&)> fetch);
};

namespace
{

auto do_unlink = [](string const& filename)
{
    ::unlink(filename.c_str());
};
typedef unity::util::ResourcePtr<string, decltype(do_unlink)> UnlinkPtr;

auto do_close = [](int fd)
{
    if (fd >= 0)
    {
        ::close(fd);
    }
};
typedef unity::util::ResourcePtr<int, decltype(do_close)> FdPtr;

}  // namespace


ThumbnailerPrivate::ThumbnailerPrivate()
{
    char const* artservice = getenv("THUMBNAILER_ART_PROVIDER");
    if (artservice && strcmp(artservice, "lastfm") == 0)
    {
        downloader_.reset(new LastFMDownloader());
    }
    else
    {
        downloader_.reset(new UbuntuServerDownloader());
    }

    string xdg_base = g_get_user_cache_dir();
    if (xdg_base == "")
    {
        string s("Thumbnailer(): Could not determine cache dir.");
        throw runtime_error(s);
    }

    string cache_dir = xdg_base + "/unity-thumbnailer";
    make_directories(cache_dir, 0700);

    try
    {
        // TODO: No good to hard-wire the cache size.
        full_size_cache_ = core::PersistentStringCache::open(cache_dir + "/images",
                                                             50 * 1024 * 1024,
                                                             core::CacheDiscardPolicy::lru_ttl);
        thumbnail_cache_ = core::PersistentStringCache::open(cache_dir + "/thumbnails",
                                                             100 * 1024 * 1024,
                                                             core::CacheDiscardPolicy::lru_ttl);
    }
    catch (std::exception const& e)
    {
        string s("Thumbnailer(): Cannot instantiate cache: ");
        s += e.what();
        throw runtime_error(s);
    }
}

string ThumbnailerPrivate::create_tmp_filename()
{
    static string dir = []
    {
        char const* dirp = getenv("TMPDIR");
        string dir = dirp ? dirp : "/tmp";
        return dir;
    }();
    static auto gen = boost::uuids::random_generator();

    string uuid = boost::lexical_cast<string>(gen());
    return dir + "/thumbnailer." + uuid + ".tmp";;
}

string ThumbnailerPrivate::read_file(string const& filename)
{
    FdPtr fd_ptr(::open(filename.c_str(), O_RDONLY), do_close);
    if (fd_ptr.get() == -1)
    {
        throw runtime_error("Thumbnailer: cannot open \"" + filename + "\": " + strerror(errno));
    }

    struct stat st;
    if (fstat(fd_ptr.get(), &st) == -1)
    {
        throw runtime_error("Thumbnailer: cannot fstat \"" + filename + "\": " + strerror(errno)); // LCOV_EXCL_LINE
    }

    string contents;
    contents.reserve(st.st_size);
    contents.resize(st.st_size);
    int rc = read(fd_ptr.get(), &contents[0], st.st_size);
    if (rc == -1)
    {
        throw runtime_error("Thumbnailer: cannot read from \"" + filename + "\": " + strerror(errno)); // LCOV_EXCL_LINE
    }
    if (rc != st.st_size)
    {
        throw runtime_error("Thumbnailer: short read for \"" + filename + "\""); // LCOV_EXCL_LINE
    }

    return contents;
}

string ThumbnailerPrivate::extract_exif_image(std::string const& filename)
{
    std::unique_ptr<ExifLoader, void (*)(ExifLoader*)> el(exif_loader_new(), exif_loader_unref);
    if (!el)
    {
        throw runtime_error("Thumbnailer::extract_exif_image(): cannot allocate ExifLoader");
    }

    exif_loader_write_file(el.get(), filename.c_str());

    std::unique_ptr<ExifData, void (*)(ExifData* e)> ed(exif_loader_get_data(el.get()), exif_data_unref);
    if (!ed || !ed->data || ed->size == 0)
    {
        return "";  // Image wasn't extracted for a reason that libexif won't tell us about.
    }

    return string(reinterpret_cast<char*>(ed->data), ed->size);
}

string ThumbnailerPrivate::extract_image_from_audio(string const& filename)
{
    UnlinkPtr tmpname(create_tmp_filename(), do_unlink);

    if (audio_.extract(filename, tmpname.get()))
    {
        return read_file(tmpname.get());
    }
    return "";
}

string ThumbnailerPrivate::extract_image_from_video(string const& filename)
{
    UnlinkPtr tmpname(create_tmp_filename(), do_unlink);

    if (video_.extract(filename, tmpname.get()))
    {
        return read_file(tmpname.get());
    }
    return "";
}

string ThumbnailerPrivate::extract_image_from_other(string const& filename)
{
    string exif_image = extract_exif_image(filename);
    if (!exif_image.empty())
    {
        return exif_image;
    }
    return read_file(filename);
}

string ThumbnailerPrivate::extract_image(string const& filename)
{
    // Work out content type.

    unique_gobj<GFile> file(g_file_new_for_path(filename.c_str()));
    if (!file)
    {
        return "";
    }

    unique_gobj<GFileInfo> info(g_file_query_info(file.get(),
                                                  G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
                                                  G_FILE_QUERY_INFO_NONE,
                                                  /* cancellable */ NULL,
                                                  /* error */ NULL));
    if (!info)
    {
        return "";
    }

    std::string content_type(g_file_info_get_content_type(info.get()));
    if (content_type.empty())
    {
        return "";
    }

    // Call the appropriate image extractor and return the image data as JPEG (not scaled).

    if (content_type.find("audio/") == 0)
    {
        cout << "EX: audio " << filename << endl;
        return extract_image_from_audio(filename);
    }
    if (content_type.find("video/") == 0)
    {
        cout << "EX: video " << filename << endl;
        return extract_image_from_video(filename);
    }
    cout << "EX: other " << filename << endl;
    return extract_image_from_other(filename);
}

Thumbnailer::Thumbnailer() : p_(new ThumbnailerPrivate())
{
}

Thumbnailer::~Thumbnailer() = default;

// Main look-up logic for thumbnails.
// key1 and key2 are set by the caller. They are artist and album or, for files,
// the pathname in key1 and the device, inode, and mtime in key2.
//
// We first look in the cache to see if we have a thumbnail already for the provided keys and size.
// If not, we check whether full-size image was downloaded previously and is still hanging
// around. If so, we scale the full-size image to what was asked for, add it to the thumbnail
// cache, and returned the scaled image. Otherwise, we fetch the image (by downloading it
// or extracting it from a file, add the full-size image to the full-size cache, scale
// the image and add the scaled version to the thumbnail cache, and return teh caled image.

string ThumbnailerPrivate::fetch_thumbnail(string const& key1,
                                           string const& key2,
                                           int desired_size,
                                           function<string(string const&, string const&)> fetch)
{
    string key = key1 + "\0" + key2;

    // desired_size is 0 if the caller wants original size.
    string sized_key = key + "\0" + to_string(desired_size);

    // Check if we have the image in the cache already.
    auto thumbnail = thumbnail_cache_->get(sized_key);
    if (thumbnail)
    {
        return *thumbnail;
    }

    // Don't have the thumbnail yet, see if we have the original image around.
    auto image = full_size_cache_->get(key);
    if (image)
    {
        cout << "found full size image with bytes: " << image->size() << endl;
        if (desired_size != 0)
        {
            Image scaled_image(*image);
            scaled_image.scale_to(desired_size);
            image = scaled_image.get_jpeg();
        }
        thumbnail_cache_->put(sized_key, *image);
        return *image;
    }

    // Try and download or read the artwork.
    cout << "cache miss, fetching" << endl;
    image = fetch(key1, key2);  // TODO: how to make async?
    if (image->empty())
    {
        // TODO: If download failed, need to disable re-try for some time.
        //       Might need to do this in the calling code, because timeouts
        //       will be different depending on why it failed, and whether
        //       the fetch was from a local or remote source.
        return "";
    }

    // We keep the full-size version around for a while because it is likely that the caller
    // will ask for small thumbnail first (for initial search results), followed by a
    // larger thumbnail (for a preview). If so, we don't download the artwork a second time.
    full_size_cache_->put(key, *image);

    if (desired_size != 0)
    {
        cout << "after cache miss, scaling" << endl;
        Image scaled_image(*image);
        scaled_image.scale_to(desired_size);
        image = scaled_image.get_jpeg();
    }
    thumbnail_cache_->put(sized_key, *image);
    return *image;
}

std::string Thumbnailer::get_thumbnail(std::string const& filename, int desired_size)
{
    assert(!filename.empty());
    assert(desired_size >= 0);

    auto path = boost::filesystem::canonical(filename);

    struct stat st;
    if (stat(path.native().c_str(), &st) == -1)
    {
        throw runtime_error("get_thumbnail(): cannot stat " + path.native() + ", errno = " + to_string(errno));
    }
    auto dev = st.st_dev;
    auto ino = st.st_ino;
    auto mtim = st.st_mtim;

    // The full cache key for the file is the concatenation of path name, device, inode, and modification time.
    // If the file exists with the same path on different removable media, or the file was modified since
    // we last cached it, the key will be different. There is no point in trying to remove such stale entries
    // from the cache. Instead, we just let the normal eviction mechanism take care of them (because stale
    // thumbnails due to file removal or file update are rare).
    string key1 = path.native();
    string key2 = to_string(dev) + "\0" + to_string(ino) + "\0" + to_string(mtim.tv_sec) + "." + to_string(mtim.tv_nsec);

    auto fetch = [this](string const& key1, string const& /* key2 */)
    {
        return p_->extract_image(key1);
    };
    cout << "calling fetch for " << key1 << endl;
    return p_->fetch_thumbnail(key1, key2, desired_size, fetch);
}

std::string Thumbnailer::get_album_art(std::string const& artist, std::string const& album, int desired_size)
{
    assert(artist.empty() || !album.empty());
    assert(album.empty() || !artist.empty());
    assert(desired_size >= 0);

    auto fetch = [this](string const& artist, string const& album)
    {
        return p_->downloader_->download(artist, album);
    };
    // Append "\0album" to key2, so we don't mix up album art and artist art.
    return p_->fetch_thumbnail(artist, album + "\0album", desired_size, fetch);
}

std::string Thumbnailer::get_artist_art(std::string const& artist, std::string const& album, int desired_size)
{
    assert(artist.empty() || !album.empty());
    assert(album.empty() || !artist.empty());
    assert(desired_size >= 0);

    auto fetch = [this](string const& artist, string const& album)
    {
        return p_->downloader_->download_artist(artist, album);
    };
    // Append "\0artist" to key2, so we don't mix up album art and artist art.
    return p_->fetch_thumbnail(artist, album + "\0artist", desired_size, fetch);
}
