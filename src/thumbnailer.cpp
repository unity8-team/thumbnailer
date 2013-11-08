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
#include<gdk-pixbuf/gdk-pixbuf.h>
#include<unistd.h>
#include<gst/gst.h>
#include<stdexcept>
#include<random>
#include<gio/gio.h>
#include<glib.h>
#include<memory>

using namespace std;

class ThumbnailerPrivate {
private:
    mt19937 rnd;

    string create_audio_thumbnail(const string &abspath, ThumbnailSize desired_size);
    string create_video_thumbnail(const string &abspath, ThumbnailSize desired_size);
    string create_generic_thumbnail(const string &abspath, ThumbnailSize desired_size);

public:
    ThumbnailCache cache;
//    AudioImageExtractor audio;
    VideoScreenshotter video;
    ImageScaler scaler;

    ThumbnailerPrivate() {};

    string create_thumbnail(const string &abspath, ThumbnailSize desired_size);
};

string ThumbnailerPrivate::create_audio_thumbnail(const string &abspath, ThumbnailSize desired_size) {
    // There was a symbol clas between 1.0 and 0.10 versions of
    // GStreamer on the desktop so we need to disable in-process
    // usage of gstreamer. Re-enable this once desktop moves to
    // newer Qt multimedia that has GStreamer 1.0.
/*
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
    */
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
    } catch(runtime_error &e) {
    }
    return "";
}

string ThumbnailerPrivate::create_video_thumbnail(const string &abspath, ThumbnailSize desired_size) {
    char filebuf[] = "/tmp/some/long/text/here/so/path/will/fit";
    string tnfile = cache.get_cache_file_name(abspath, desired_size);
    string tmpname = tmpnam(filebuf);
    if(video.extract(abspath, tmpname)) {
        scaler.scale(tmpname, tnfile, desired_size, abspath);
        unlink(tmpname.c_str());
        return tnfile;
    }
    throw runtime_error("Video extraction failed.");
}

string ThumbnailerPrivate::create_thumbnail(const string &abspath, ThumbnailSize desired_size) {
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
        return create_audio_thumbnail(abspath, desired_size);
    }

    if (content_type.find("video/") == 0) {
        return create_video_thumbnail(abspath, desired_size);
    }

    return create_generic_thumbnail(abspath, desired_size);
}

Thumbnailer::Thumbnailer() {
    p = new ThumbnailerPrivate();
}

Thumbnailer::~Thumbnailer() {
    delete p;
}

string Thumbnailer::get_thumbnail(const string &filename, ThumbnailSize desired_size) {
    string abspath;
    if(filename[0] != '/') {
        abspath += getcwd(nullptr, 0);
        abspath += "/" + filename;
    } else {
        abspath = filename;
    }
    std::string estimate = p->cache.get_if_exists(abspath, desired_size);
    if(!estimate.empty())
        return estimate;
    string generated = p->create_thumbnail(abspath, desired_size);
    if(generated == abspath) {
        return abspath;
    }
    return p->cache.get_if_exists(abspath, desired_size);
}
