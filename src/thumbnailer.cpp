/*
 * Copyright (C) 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Jussi Pakkanen <jussi.pakkanen@canonical.com>
 */

#include<thumbnailer.h>
#include<internal/thumbnailcache.h>
#include<unistd.h>
#include<gst/gst.h>

using std::string;

class GstInitializer {
public:
    GstInitializer() { gst_init(nullptr, nullptr); };
};

class ThumbnailerPrivate {
public:
    ThumbnailCache cache;
    ThumbnailerPrivate() {};

    string create_thumbnail(const string &abspath, ThumbnailSize desired_size);
};

string ThumbnailerPrivate::create_thumbnail(const string &abspath, ThumbnailSize desired_size) {

}

Thumbnailer::Thumbnailer() {
    static GstInitializer i; // C++ standard guarantees this to be lazy and thread safe.
    p = new ThumbnailerPrivate();
}

Thumbnailer::~Thumbnailer() {
    delete p;
}

string Thumbnailer::get_thumbnail(const string &filename, ThumbnailSize desired_size) {
    string abspath;
    if(filename[0] != '/') {
      abspath += getcwd(nullptr, 0) + '/' + filename;
    } else {
        abspath = filename;
    }

    std::string estimate = p->cache.get_if_exists(filename, desired_size);
    if(!estimate.empty())
        return estimate;
    p->create_thumbnail(filename, desired_size);
    return p->cache.get_if_exists(filename, desired_size);
}
