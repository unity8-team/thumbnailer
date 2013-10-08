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

using namespace std;

class ThumbnailerPrivate {
private:
    mt19937 rnd;

public:
    ThumbnailCache cache;
//    AudioImageExtractor audio;
    VideoScreenshotter video;
    ImageScaler scaler;

    ThumbnailerPrivate() {};

    string create_thumbnail(const string &abspath, ThumbnailSize desired_size);
};

string ThumbnailerPrivate::create_thumbnail(const string &abspath, ThumbnailSize desired_size) {
    int tmpw, tmph;
    // Every now and then see if we have too much stuff and delete them if so.
    if((rnd() % 100) == 0) { // No, this is not perfectly random. It does not need to be.
        cache.prune();
    }
    // We do not know what the file pointed to is. File suffixes may be missing
    // or just plain lie. So we try to open it one by one with the handlers we have.
    // Go from least resource intensive to most.
    string tnfile = cache.get_cache_file_name(abspath, desired_size);
    char filebuf[] = "/tmp/some/long/text/here/so/path/will/fit";
    string tmpname = tmpnam(filebuf);

    // Special case: full size image files are their own preview.
    if(desired_size == TN_SIZE_ORIGINAL &&
            gdk_pixbuf_get_file_info(abspath.c_str(), &tmpw, &tmph)) {
        return abspath;
    }
    try {
        if(scaler.scale(abspath, tnfile, desired_size, abspath))
            return tnfile;
    } catch(runtime_error &e) {
        // Fail is ok, just try the next one.
    }
    bool extracted = false;
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
    try {
        if(video.extract(abspath, tmpname)) {
            extracted = true;
        }
    } catch(runtime_error &e) {
    }
    if(extracted) {
        scaler.scale(tmpname, tnfile, desired_size, abspath); // If this throws, let it propagate.
        unlink(tmpname.c_str());
        return tnfile;
    }
    // None of our handlers knew how to handle it.
    return "";
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
