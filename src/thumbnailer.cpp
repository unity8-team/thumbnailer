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

#include<thumbnailer.h>
#include<internal/thumbnailcache.h>
#include<internal/audioimageextractor.h>
#include<internal/videoscreenshotter.h>
#include<internal/imagescaler.h>
#include<internal/mediaartcache.h>
#include<internal/lastfmdownloader.h>
#include<internal/ubuntuserverdownloader.h>
#include<gdk-pixbuf/gdk-pixbuf.h>
#include<libexif/exif-loader.h>
#include<unistd.h>
#include<cstring>
#include<gst/gst.h>
#include<stdexcept>
#include<random>
#include<gio/gio.h>
#include<glib.h>
#include<memory>

using namespace std;

class ThumbnailerPrivate {
private:
    random_device rnd;

    string create_audio_thumbnail(const string &abspath, ThumbnailSize desired_size,
            ThumbnailPolicy policy);
    string create_video_thumbnail(const string &abspath, ThumbnailSize desired_size);
    string create_generic_thumbnail(const string &abspath, ThumbnailSize desired_size);

public:
    ThumbnailCache cache;
    AudioImageExtractor audio;
    VideoScreenshotter video;
    ImageScaler scaler;
    MediaArtCache macache;
    std::unique_ptr<ArtDownloader> downloader;

    ThumbnailerPrivate() {
        char *artservice = getenv("THUMBNAILER_ART_PROVIDER");
        if (artservice != nullptr && strcmp(artservice, "lastfm") == 0)
        {
            downloader.reset(new LastFMDownloader());
        }
        else
        {
            downloader.reset(new UbuntuServerDownloader());
        }
    };

    string create_thumbnail(const string &abspath, ThumbnailSize desired_size,
            ThumbnailPolicy policy);
    string create_random_filename();
    string extract_exif_thumbnail(const std::string &abspath);
};

string ThumbnailerPrivate::create_random_filename() {
    string fname;
    char *dirbase = getenv("TMPDIR"); // Set when in a confined application.
    if(dirbase) {
        fname = dirbase;
    } else {
        fname = "/tmp";
    }
    fname += "/thumbnailer.";
    fname += to_string(rnd());
    fname += ".tmp";
    return fname;
}

string ThumbnailerPrivate::extract_exif_thumbnail(const std::string &abspath) {
    std::unique_ptr<ExifLoader, void(*)(ExifLoader*)>el(exif_loader_new(), exif_loader_unref);
    if(el) {
        exif_loader_write_file(el.get(), abspath.c_str());
        std::unique_ptr<ExifData, void(*)(ExifData*e)> ed(exif_loader_get_data(el.get()), exif_data_unref);
        if(ed && ed->data && ed->size) {
            auto outfile = create_random_filename();
            FILE *thumb = fopen(outfile.c_str(), "wb");
            if(thumb) {
                fwrite(ed->data, 1, ed->size, thumb);
                fclose(thumb);
                return outfile;
            }
        }
    }
    return "";
}

string ThumbnailerPrivate::create_audio_thumbnail(const string &abspath,
        ThumbnailSize desired_size, ThumbnailPolicy /*policy*/) {
    string tnfile = cache.get_cache_file_name(abspath, desired_size);
    string tmpname = create_random_filename();
    bool extracted = false;
    try {
        if(audio.extract(abspath, tmpname)) {
            extracted = true;
        }
    } catch(runtime_error &e) {
    }
    if(extracted) {
        scaler.scale(tmpname, tnfile, desired_size, abspath); // If this throws, let it propagate.
        unlink(tmpname.c_str());
        return tnfile;
    }
    return "";
}
string ThumbnailerPrivate::create_generic_thumbnail(const string &abspath, ThumbnailSize desired_size) {
    int tmpw, tmph;
    string tnfile = cache.get_cache_file_name(abspath, desired_size);
    // Special case: full size image files are their own preview.
    if(desired_size == TN_SIZE_ORIGINAL &&
       gdk_pixbuf_get_file_info(abspath.c_str(), &tmpw, &tmph)) {
        return abspath;
    }
    try {
        if(scaler.scale(abspath, tnfile, desired_size, abspath))
            return tnfile;
    } catch(const runtime_error &e) {
        fprintf(stderr, "Scaling thumbnail failed: %s\n", e.what());
    }
    return "";
}

string ThumbnailerPrivate::create_video_thumbnail(const string &abspath, ThumbnailSize desired_size) {
    string tnfile = cache.get_cache_file_name(abspath, desired_size);
    string tmpname = create_random_filename();
    if(video.extract(abspath, tmpname)) {
        scaler.scale(tmpname, tnfile, desired_size, abspath);
        unlink(tmpname.c_str());
        return tnfile;
    }
    throw runtime_error("Video extraction failed.");
}

string ThumbnailerPrivate::create_thumbnail(const string &abspath, ThumbnailSize desired_size,
        ThumbnailPolicy policy) {
    // Every now and then see if we have too much stuff and delete them if so.
    if((rnd() % 100) == 0) { // No, this is not perfectly random. It does not need to be.
        cache.prune();
    }
    std::unique_ptr<GFile, void(*)(void *)> file(
            g_file_new_for_path(abspath.c_str()), g_object_unref);
    if(!file) {
        return "";
    }

    std::unique_ptr<GFileInfo, void(*)(void *)> info(
            g_file_query_info(file.get(), G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
                G_FILE_QUERY_INFO_NONE, /* cancellable */ NULL, /* error */NULL),
            g_object_unref);
    if(!info) {
        return "";
    }

    std::string content_type(g_file_info_get_content_type(info.get()));
    if (content_type.empty()) {
        return "";
    }

    if (content_type.find("audio/") == 0) {
        return create_audio_thumbnail(abspath, desired_size, policy);
    }

    if (content_type.find("video/") == 0) {
        return create_video_thumbnail(abspath, desired_size);
    }
    if(desired_size != TN_SIZE_ORIGINAL) {
        try {
            auto embedded_image = extract_exif_thumbnail(abspath);
            // Note that we always use the embedded thumbnail if it exists.
            // Depending on usage, it may be too low res for our purposes.
            // If this becomes an issue, add a check here once we know the
            // real resolution requirements and fall back to scaling the
            // full image if the thumbnail is too small.
            if(!embedded_image.empty()) {
                string tnfile = cache.get_cache_file_name(abspath, desired_size);
                auto succeeded = scaler.scale(embedded_image, tnfile, desired_size, abspath, abspath);
                unlink(embedded_image.c_str());
                if(succeeded) {
                    return tnfile;
                }
            }
        } catch(const exception &e) {
        }
    }

    return create_generic_thumbnail(abspath, desired_size);
}

Thumbnailer::Thumbnailer() {
    p = new ThumbnailerPrivate();
}

Thumbnailer::~Thumbnailer() {
    delete p;
}
std::string Thumbnailer::get_thumbnail(const std::string &filename, ThumbnailSize desired_size,
        ThumbnailPolicy policy) {
    string abspath;
    if(filename.empty()) {
        return "";
    }
    if(filename[0] != '/') {
        auto cwd = getcwd(nullptr, 0);
        abspath += cwd;
        free(cwd);
        abspath += "/" + filename;
    } else {
        abspath = filename;
    }
    std::string estimate = p->cache.get_if_exists(abspath, desired_size);
    if(!estimate.empty())
        return estimate;

    if (p->cache.has_failure(abspath)) {
        throw runtime_error("Thumbnail generation failed previously, not trying again.");
    }

    try {
        string generated = p->create_thumbnail(abspath, desired_size, policy);
        if(generated == abspath) {
            return abspath;
        }
    } catch(std::runtime_error &e) {
        p->cache.mark_failure(abspath);
        throw;
    }

    return p->cache.get_if_exists(abspath, desired_size);
}

string Thumbnailer::get_thumbnail(const string &filename, ThumbnailSize desired_size) {
    return get_thumbnail(filename, desired_size, TN_LOCAL);
}

unique_ptr<gchar, void(*)(gpointer)> get_art_file_content(const string &fname, gsize &content_size)
{
    gchar *contents;
    GError *err = nullptr;
    if(!g_file_get_contents(fname.c_str(), &contents, &content_size, &err)) {
        unlink(fname.c_str());
        std::string msg("Error reading file: ");
        msg += err->message;
        g_error_free(err);
        throw std::runtime_error(msg);
    }
    unlink(fname.c_str());
    return unique_ptr<gchar, void(*)(gpointer)>(contents, g_free);
}

std::string Thumbnailer::get_album_art(const std::string &artist, const std::string &album,
        ThumbnailSize desired_size, ThumbnailPolicy policy) {
    if(!p->macache.has_album_art(artist, album)) {
        if(policy == TN_LOCAL) {
            // We don't have it cached and can't access the net
            // -> nothing to be done.
            return "";
        }
        char filebuf[] = "/tmp/some/long/text/here/so/path/will/fit";
        std::string tmpname = tmpnam(filebuf);
        if(!p->downloader->download(artist, album, tmpname)) {
            return "";
        }

        gsize content_size;
        auto contents = get_art_file_content(tmpname, content_size);
        p->macache.add_album_art(artist, album, contents.get(), content_size);
    }
    // At this point we know we have the image in our art cache (unless
    // someone just deleted it concurrently, in which case we can't
    // really do anything.
    std::string original = p->macache.get_album_art_file(artist, album);
    if(desired_size == TN_SIZE_ORIGINAL) {
        return original;
    }
    return get_thumbnail(original, desired_size, policy);
}

std::string Thumbnailer::get_artist_art(const std::string &artist, const std::string &album, ThumbnailSize desired_size,
        ThumbnailPolicy policy) {
    if(!p->macache.has_artist_art(artist, album)) {
        if(policy == TN_LOCAL) {
            // We don't have it cached and can't access the net
            // -> nothing to be done.
            return "";
        }
        char filebuf[] = "/tmp/some/long/text/here/so/path/will/fit";
        std::string tmpname = tmpnam(filebuf);
        if(!p->downloader->download_artist(artist, album, tmpname)) {
            return "";
        }
        gsize content_size;
        auto contents = get_art_file_content(tmpname, content_size);
        p->macache.add_artist_art(artist, album, contents.get(), content_size);
    }
    // At this point we know we have the image in our art cache (unless
    // someone just deleted it concurrently, in which case we can't
    // really do anything.
    std::string original = p->macache.get_artist_art_file(artist, album);
    if(desired_size == TN_SIZE_ORIGINAL) {
        return original;
    }
    return get_thumbnail(original, desired_size, policy);

    return "";
}
