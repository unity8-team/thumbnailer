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

#include<internal/imagescaler.h>
#include<gdk-pixbuf/gdk-pixbuf.h>
#include<memory>

using namespace std;

class ImageScalerPrivate {
};

ImageScaler::ImageScaler() {
    p = new ImageScalerPrivate();
}

ImageScaler::~ImageScaler() {
    delete p;
}

bool ImageScaler::scale(std::string &ifilename, std::string &ofilename, ThumbnailSizes wanted) const {
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
    if(!gdk_pixbuf_save(dst.get(), ofilename.c_str(), "jpeg", &err, NULL)) {
        string msg = err->message;
        g_error_free(err);
        throw runtime_error(msg);
    }
    return true;
}
