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
#include <internal/imagescaler.h>
#include <internal/lastfmdownloader.h>
#include <internal/thumbnailcache.h>
#include <internal/ubuntuserverdownloader.h>
#include <internal/videoscreenshotter.h>

#include <boost/filesystem.hpp>
#include <core/persistent_string_cache.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include <libexif/exif-loader.h>

#include <gst/gst.h>

#include <gio/gio.h>

#include <glib.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <unity/util/ResourcePtr.h>

#include <cstring>
#include <fstream>
#include <memory>
#include <unistd.h>
#include <random>
#include <stdexcept>

using namespace std;
using namespace unity::thumbnailer::internal;

class ThumbnailerPrivate
{
private:
    random_device rnd;
    string cache_dir;

    string create_audio_thumbnail(string const& abspath, int desired_size, ThumbnailPolicy policy);
    string create_video_thumbnail(string const& abspath, int desired_size);
    string create_generic_thumbnail(string const& abspath, int desired_size);

public:
    ThumbnailCache cache;
    AudioImageExtractor audio_;
    VideoScreenshotter video_;
    ImageScaler scaler_;
    core::PersistentStringCache::UPtr full_size_cache_;  // Small cache of full (original) size images.
    core::PersistentStringCache::UPtr thumbnail_cache_;  // Large cache of scaled images.
    std::unique_ptr<ArtDownloader> downloader_;

    ThumbnailerPrivate()
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

        if (mkdir(xdg_base.c_str(), S_IRUSR | S_IWUSR | S_IXUSR) == -1)
        {
            if (errno != EEXIST)
            {
                string s("Thumbnailer(): Could not create base dir.");
                throw runtime_error(s);
            }
        }
        else
        {
            chmod(xdg_base.c_str(), 0700);
        }

        cache_dir = xdg_base + "/unity-thumbnailer";
        if (mkdir(cache_dir.c_str(), S_IRUSR | S_IWUSR | S_IXUSR) == -1)
        {
            if (errno != EEXIST)
            {
                string s("Thumbnailer(): Could not create cache dir.");
                throw runtime_error(s);
            }
        }
        else
        {
            chmod(cache_dir.c_str(), 0700);
        }

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
    };

    string extract_image(string const& abspath, int desired_size, ThumbnailPolicy policy);
    string create_random_filename();
    string extract_exif_thumbnail(std::string const& abspath);
    string fetch_thumbnail(string const& key1,
                           string const& key2,
                           int desired_size,
                           ThumbnailPolicy policy,
                           function<string(string const&, string const&)> fetch);
};

string ThumbnailerPrivate::create_random_filename()
{
    char* dirbase = getenv("TMPDIR");  // Set when in a confined application.
    string fname = dirbase ? dirbase : "/tmp";
    fname += "/thumbnailer.";
    fname += to_string(rnd());
    fname += ".tmp";
    return fname;
}

string ThumbnailerPrivate::extract_exif_thumbnail(std::string const& abspath)
{
    std::unique_ptr<ExifLoader, void (*)(ExifLoader*)> el(exif_loader_new(), exif_loader_unref);
    if (el)
    {
        exif_loader_write_file(el.get(), abspath.c_str());
        std::unique_ptr<ExifData, void (*)(ExifData* e)> ed(exif_loader_get_data(el.get()), exif_data_unref);
        if (ed && ed->data && ed->size)
        {
            auto outfile = create_random_filename();
            FILE* thumb = fopen(outfile.c_str(), "wb");
            if (thumb)
            {
                fwrite(ed->data, 1, ed->size, thumb);
                fclose(thumb);
                return outfile;
            }
        }
    }
    return "";
}

string ThumbnailerPrivate::create_audio_thumbnail(string const& abspath,
                                                  int desired_size,
                                                  ThumbnailPolicy /*policy*/)
{
    string tnfile = cache.get_cache_file_name(abspath, desired_size);
    string tmpname = create_random_filename();
    bool extracted = false;
    try
    {
        if (audio_.extract(abspath, tmpname))
        {
            extracted = true;
        }
    }
    catch (runtime_error& e)
    {
    }
    if (extracted)
    {
        scaler_.scale(tmpname, tnfile, desired_size, abspath);  // If this throws, let it propagate.
        unlink(tmpname.c_str());
        return tnfile;
    }
    return "";
}
string ThumbnailerPrivate::create_generic_thumbnail(string const& abspath, int desired_size)
{
    int tmpw, tmph;
    string tnfile = cache.get_cache_file_name(abspath, desired_size);
    // Special case: full size image files are their own preview.
    if (desired_size == 0 && gdk_pixbuf_get_file_info(abspath.c_str(), &tmpw, &tmph))
    {
        return abspath;
    }
    try
    {
        if (scaler_.scale(abspath, tnfile, desired_size, abspath))
        {
            return tnfile;
        }
    }
    catch (runtime_error const& e)
    {
        fprintf(stderr, "Scaling thumbnail failed: %s\n", e.what());
    }
    return "";
}

string ThumbnailerPrivate::create_video_thumbnail(string const& abspath, int desired_size)
{
    string tnfile = cache.get_cache_file_name(abspath, desired_size);
    string tmpname = create_random_filename();
    if (video_.extract(abspath, tmpname))
    {
        scaler_.scale(tmpname, tnfile, desired_size, abspath);
        unlink(tmpname.c_str());
        return tnfile;
    }
    throw runtime_error("Video extraction failed.");
}

string ThumbnailerPrivate::extract_image(string const& abspath, int desired_size, ThumbnailPolicy policy)
{
    unique_gobj<GFile> file(g_file_new_for_path(abspath.c_str()));
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

    if (content_type.find("audio/") == 0)
    {
        return create_audio_thumbnail(abspath, desired_size, policy);
    }

    if (content_type.find("video/") == 0)
    {
        return create_video_thumbnail(abspath, desired_size);
    }
    if (desired_size != 0)
    {
        try
        {
            auto embedded_image = extract_exif_thumbnail(abspath);
            // Note that we always use the embedded thumbnail if it exists.
            // Depending on usage, it may be too low res for our purposes.
            // If this becomes an issue, add a check here once we know the
            // real resolution requirements and fall back to scaling the
            // full image if the thumbnail is too small.
            if (!embedded_image.empty())
            {
                string tnfile = cache.get_cache_file_name(abspath, desired_size);
                auto succeeded = scaler_.scale(embedded_image, tnfile, desired_size, abspath, abspath);
                unlink(embedded_image.c_str());
                if (succeeded)
                {
                    return tnfile;
                }
            }
        }
        catch (exception const& e)
        {
        }
    }

    return create_generic_thumbnail(abspath, desired_size);
}

Thumbnailer::Thumbnailer() : p_(new ThumbnailerPrivate())
{
}

Thumbnailer::~Thumbnailer() = default;

#if 0
std::string Thumbnailer::get_thumbnail(std::string const& filename, int desired_size, ThumbnailPolicy policy)
{
    assert(desired_size >= 0);

    string abspath;
    if (filename.empty())
    {
        return "";
    }
    if (filename[0] != '/')
    {
        auto cwd = getcwd(nullptr, 0);
        abspath += cwd;
        free(cwd);
        abspath += "/" + filename;
    }
    else
    {
        abspath = filename;
    }
    std::string estimate = p_->cache.get_if_exists(abspath, desired_size);
    if (!estimate.empty())
    {
        return estimate;
    }

    if (p_->cache.has_failure(abspath))
    {
        throw runtime_error("Thumbnail generation failed previously, not trying again.");
    }

    try
    {
        string generated = p_->create_thumbnail(abspath, desired_size, policy);
        if (generated == abspath)
        {
            return abspath;
        }
    }
    catch (std::runtime_error const& e)
    {
        p_->cache.mark_failure(abspath);
        throw;
    }

    return p_->cache.get_if_exists(abspath, desired_size);
}
#endif

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
                                           ThumbnailPolicy policy,
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
        if (desired_size != 0)
        {
            image = image;  // TODO: scale here
        }
        thumbnail_cache_->put(sized_key, *image);
        return *image;
    }

    if (policy == TN_LOCAL)
    {
        // We don't have either original or scaled version around and can't download it.
        return "";
    }

    // Try and download or read the artwork.
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

    image = image;  // TODO: scale here
    thumbnail_cache_->put(sized_key, *image);
    return *image;
}

namespace
{

typedef unity::util::ResourcePtr<int, decltype(&::close)> FdPtr;

}

std::string Thumbnailer::get_thumbnail(std::string const& filename, int desired_size, ThumbnailPolicy policy)
{
    assert(!filename.empty());
    assert(desired_size >= 0);

    using namespace boost::filesystem;

    auto path = canonical(filename);

    // Open and stat the file, and get device, inode, and modification time.
    int fd = open(path.native().c_str(), O_RDONLY);
    if (fd == -1)
    {
        throw runtime_error("get_thumbnail(): cannot open " + path.native() + ", errno = " + to_string(errno));
    }
    FdPtr fd_ptr(fd, ::close);
    struct stat st;
    if (fstat(fd_ptr.get(), &st) == -1)
    {
        throw runtime_error("get_thumbnail(): cannot fstat " + path.native() + ", fd = " + to_string(fd) +
                            ", errno = " + to_string(errno));
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

    auto fetch = [this, fd](string const& key1, string const& key2)
    {
        // TODO open file, extract thumbnail
        return string("");  // TODO
    };
    return p_->fetch_thumbnail(key1, key2, desired_size, policy, fetch);
}

std::string Thumbnailer::get_album_art(std::string const& artist,
                                       std::string const& album,
                                       int desired_size,
                                       ThumbnailPolicy policy)
{
    assert(artist.empty() || !album.empty());
    assert(album.empty() || !artist.empty());
    assert(desired_size >= 0);

    auto fetch = [this](string const& artist, string const& album)
    {
        return p_->downloader_->download(artist, album);
    };
    // Append "\0album" to key2, so we don't mix up album art and artist art.
    return p_->fetch_thumbnail(artist, album + "\0album", desired_size, policy, fetch);
}

std::string Thumbnailer::get_artist_art(std::string const& artist,
                                        std::string const& album,
                                        int desired_size,
                                        ThumbnailPolicy policy)
{
    assert(artist.empty() || !album.empty());
    assert(album.empty() || !artist.empty());
    assert(desired_size >= 0);

    auto fetch = [this](string const& artist, string const& album)
    {
        return p_->downloader_->download_artist(artist, album);
    };
    // Append "\0artist" to key2, so we don't mix up album art and artist art.
    return p_->fetch_thumbnail(artist, album + "\0artist", desired_size, policy, fetch);
}
