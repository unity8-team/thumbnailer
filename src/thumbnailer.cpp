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
 */

#include <thumbnailer.h>

#include <internal/audioimageextractor.h>
#include <internal/imagescaler.h>
#include <internal/lastfmdownloader.h>
#include <internal/thumbnailcache.h>
#include <internal/ubuntuserverdownloader.h>
#include <internal/videoscreenshotter.h>

#include <core/persistent_string_cache.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include <libexif/exif-loader.h>

#include <gst/gst.h>

#include <gio/gio.h>

#include <glib.h>

#include <sys/stat.h>

#include <cstring>
#include <fstream>
#include <memory>
#include <unistd.h>
#include <random>
#include <stdexcept>

using namespace std;
using namespace unity::thumbnailer::internal;

void store_to_file(string const& content, string const& file_path)
{
    ofstream out_stream(file_path);
    out_stream << content;
    out_stream.close();
}

class ThumbnailerPrivate
{
private:
    random_device rnd;
    string cache_dir;

    string create_audio_thumbnail(string const& abspath, ThumbnailSize desired_size, ThumbnailPolicy policy);
    string create_video_thumbnail(string const& abspath, ThumbnailSize desired_size);
    string create_generic_thumbnail(string const& abspath, ThumbnailSize desired_size);

public:
    ThumbnailCache cache;
    AudioImageExtractor audio;
    VideoScreenshotter video;
    ImageScaler scaler;
    core::PersistentStringCache::UPtr macache;
    std::unique_ptr<ArtDownloader> downloader;

    ThumbnailerPrivate()
    {
        char* artservice = getenv("THUMBNAILER_ART_PROVIDER");
        if (artservice != nullptr && strcmp(artservice, "lastfm") == 0)
        {
            downloader.reset(new LastFMDownloader());
        }
        else
        {
            downloader.reset(new UbuntuServerDownloader());
        }

        string xdg_base = g_get_user_cache_dir();

        if (xdg_base == "") {
            string s("Could not determine cache dir.");
            throw runtime_error(s);
        }

        int ec = mkdir(xdg_base.c_str(), S_IRUSR | S_IWUSR | S_IXUSR);
        if (ec < 0 && errno != EEXIST) {
            string s("Could not create base dir.");
            throw runtime_error(s);
        }
        cache_dir = xdg_base + "/media-art";
        ec = mkdir(cache_dir.c_str(), S_IRUSR | S_IWUSR | S_IXUSR);
        if (ec < 0 && errno != EEXIST) {
            string s("Could not create cache dir.");
            throw runtime_error(s);
        }

        // TODO: No good to hard-wire the cache size. 100 MB will do for now.
        macache = core::PersistentStringCache::open(cache_dir, 100 * 1024 * 1024, core::CacheDiscardPolicy::lru_only);
    };

    string create_thumbnail(string const& abspath, ThumbnailSize desired_size, ThumbnailPolicy policy);
    string create_random_filename();
    string extract_exif_thumbnail(std::string const& abspath);
};

string ThumbnailerPrivate::create_random_filename()
{
    string fname;
    char* dirbase = getenv("TMPDIR");  // Set when in a confined application.
    if (dirbase)
    {
        fname = dirbase;
    }
    else
    {
        fname = "/tmp";
    }
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
                                                  ThumbnailSize desired_size,
                                                  ThumbnailPolicy /*policy*/)
{
    string tnfile = cache.get_cache_file_name(abspath, desired_size);
    string tmpname = create_random_filename();
    bool extracted = false;
    try
    {
        if (audio.extract(abspath, tmpname))
        {
            extracted = true;
        }
    }
    catch (runtime_error& e)
    {
    }
    if (extracted)
    {
        scaler.scale(tmpname, tnfile, desired_size, abspath);  // If this throws, let it propagate.
        unlink(tmpname.c_str());
        return tnfile;
    }
    return "";
}
string ThumbnailerPrivate::create_generic_thumbnail(string const& abspath, ThumbnailSize desired_size)
{
    int tmpw, tmph;
    string tnfile = cache.get_cache_file_name(abspath, desired_size);
    // Special case: full size image files are their own preview.
    if (desired_size == TN_SIZE_ORIGINAL && gdk_pixbuf_get_file_info(abspath.c_str(), &tmpw, &tmph))
    {
        return abspath;
    }
    try
    {
        if (scaler.scale(abspath, tnfile, desired_size, abspath))
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

string ThumbnailerPrivate::create_video_thumbnail(string const& abspath, ThumbnailSize desired_size)
{
    string tnfile = cache.get_cache_file_name(abspath, desired_size);
    string tmpname = create_random_filename();
    if (video.extract(abspath, tmpname))
    {
        scaler.scale(tmpname, tnfile, desired_size, abspath);
        unlink(tmpname.c_str());
        return tnfile;
    }
    throw runtime_error("Video extraction failed.");
}

string ThumbnailerPrivate::create_thumbnail(string const& abspath, ThumbnailSize desired_size, ThumbnailPolicy policy)
{
    // Every now and then see if we have too much stuff and delete them if so.
    if ((rnd() % 100) == 0)  // No, this is not perfectly random. It does not need to be.
    {
        cache.prune();
    }
    std::unique_ptr<GFile, void (*)(void*)> file(g_file_new_for_path(abspath.c_str()), g_object_unref);
    if (!file)
    {
        return "";
    }

    std::unique_ptr<GFileInfo, void (*)(void*)> info(g_file_query_info(file.get(),
                                                                       G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
                                                                       G_FILE_QUERY_INFO_NONE,
                                                                       /* cancellable */ NULL,
                                                                       /* error */ NULL),
                                                     g_object_unref);
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
    if (desired_size != TN_SIZE_ORIGINAL)
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
                auto succeeded = scaler.scale(embedded_image, tnfile, desired_size, abspath, abspath);
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

std::string Thumbnailer::get_thumbnail(std::string const& filename, ThumbnailSize desired_size, ThumbnailPolicy policy)
{
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

string Thumbnailer::get_thumbnail(string const& filename, ThumbnailSize desired_size)
{
    return get_thumbnail(filename, desired_size, TN_LOCAL);
}

std::string Thumbnailer::get_album_art(std::string const& artist,
                                       std::string const& album,
                                       ThumbnailSize /* desired_size */,
                                       ThumbnailPolicy policy)
{
    string key = artist + album + "album";
    auto thumbnail = p_->macache->get(key);
    if (thumbnail)
    {
        return *thumbnail;
    }
    if (policy == TN_LOCAL)
    {
        // We don't have it cached and can't access the net
        // -> nothing to be done.
        return "";
    }
    std::string image = p_->downloader->download(artist, album);
    if (image.empty())
    {
        return "";
    }
    p_->macache->put(key, image);
    return image;
}

std::string Thumbnailer::get_artist_art(std::string const& artist,
                                        std::string const& album,
                                        ThumbnailSize /* desired_size */,
                                        ThumbnailPolicy policy)
{
    string key = artist + album + "artist";
    auto thumbnail = p_->macache->get(key);
    if (thumbnail)
    {
        return *thumbnail;
    }
    if (policy == TN_LOCAL)
    {
        // We don't have it cached and can't access the net
        // -> nothing to be done.
        return "";
    }
    std::string image = p_->downloader->download_artist(artist, album);
    if (image.empty())
    {
        return "";
    }
    p_->macache->put(key, image);
    return image;
}
