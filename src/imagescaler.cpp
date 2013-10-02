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

#include<internal/imagescaler.h>
#include<gdk-pixbuf/gdk-pixbuf.h>
#include<memory>
#include<stdexcept>
#include<sys/stat.h>
#include<cassert>

using namespace std;

class ImageScalerPrivate {
};

ImageScaler::ImageScaler() {
    p = new ImageScalerPrivate();
}

ImageScaler::~ImageScaler() {
    delete p;
}

bool ImageScaler::scale(const std::string &ifilename, const std::string &ofilename, ThumbnailSize wanted,
        const std::string &original_location) const {
    assert(ifilename[0] == '/');
    assert(ofilename[0] == '/');
    GError *err = nullptr;
    auto pbunref = [](GdkPixbuf *t) { g_object_unref(G_OBJECT(t)); };
    int max_dim = wanted == TN_SIZE_SMALL ? 128 : 256;
    unique_ptr<GdkPixbuf, void(*)(GdkPixbuf *t)> src(
            gdk_pixbuf_new_from_file(ifilename.c_str(), &err),
            pbunref);
    if(err) {
        string msg = err->message;
        g_error_free(err);
        throw runtime_error(msg);
    }
    const int w = gdk_pixbuf_get_width(src.get());
    const int h = gdk_pixbuf_get_height(src.get());
    int neww, newh;
    if(w == 0 || h == 0) {
        throw runtime_error("Invalid image resolution.");
    }
    if(w > h) {
        neww = max_dim;
        newh = ((double)(max_dim))*h/w;
        if (newh == 0)
            newh = 1;
    } else {
        newh = max_dim;
        neww = ((double)(max_dim))*w/h;
        if(neww == 0)
            neww = 1;
    }
    unique_ptr<GdkPixbuf, void(*)(GdkPixbuf *t)> dst(
            gdk_pixbuf_scale_simple(src.get(), neww, newh, GDK_INTERP_BILINEAR),
            pbunref);
    gboolean save_ok;
    if(original_location.empty()) {
        save_ok = gdk_pixbuf_save(dst.get(), ofilename.c_str(), "png", &err, NULL);
    } else {
        assert(original_location[0] == '/');
        time_t mtime;
        string uri = "file://" + original_location;
        struct stat sbuf;
        if(stat(original_location.c_str(), &sbuf) != 0) {
            mtime = 0;
        } else {
            mtime = sbuf.st_mtim.tv_sec;
        }
        string mtime_str = to_string(mtime);
        save_ok = gdk_pixbuf_save(dst.get(), ofilename.c_str(), "png", &err,
                "tEXt::Thumb::URI", uri.c_str(), "tEXt::Thumb::MTime", mtime_str.c_str(), NULL);
    }
    if(!save_ok) {
        string msg = err->message;
        g_error_free(err);
        throw runtime_error(msg);
    }
    return true;
}
