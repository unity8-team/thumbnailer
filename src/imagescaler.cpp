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
#include<internal/gobj_memory.h>
#include<gdk-pixbuf/gdk-pixbuf.h>
#include<libexif/exif-loader.h>
#include<memory>
#include<stdexcept>
#include<sys/stat.h>
#include<cassert>
#include<random>
#include<cstring>

using namespace std;

static void determine_new_size(const int w, const int h, int &neww, int &newh,
        const ThumbnailSize wanted) {
    if(wanted == TN_SIZE_ORIGINAL) {
        neww = w;
        newh = h;
    } else {
        int max_dim = ((wanted == TN_SIZE_SMALL) ? 128 : ((wanted == TN_SIZE_LARGE) ? 256 : 512));
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
    }
}

static GdkPixbuf* fix_orientation(const char *infile, GdkPixbuf *src) {
    ExifLoader *l;
    l = exif_loader_new();
    ExifData *ed;
    exif_loader_write_file(l, infile);
    ed = exif_loader_get_data(l);
    exif_loader_unref(l);
    GdkPixbuf *rot = nullptr;
    // Have to do this manually because of
    // https://bugzilla.gnome.org/show_bug.cgi?id=725582
    if(ed) {
      ExifEntry *e = exif_data_get_entry(ed, EXIF_TAG_ORIENTATION);
      if(e) {
          int orientation = exif_get_short(e->data, exif_data_get_byte_order(ed));
          if(orientation == 6) {
              rot = gdk_pixbuf_rotate_simple(src, GDK_PIXBUF_ROTATE_CLOCKWISE);
          } else if (orientation == 8) {
              rot = gdk_pixbuf_rotate_simple(src, GDK_PIXBUF_ROTATE_COUNTERCLOCKWISE);
          } else if (orientation == 3) {
              rot = gdk_pixbuf_rotate_simple(src, GDK_PIXBUF_ROTATE_UPSIDEDOWN);
          } else {
              // We don't do mirrored images, at least not yet.
          }
      }
    }
    if(!rot) {
        // This entire function should only contain this call, really.
      rot = gdk_pixbuf_apply_embedded_orientation(src);
    }
    exif_data_unref(ed);
    return rot;
}

class ImageScalerPrivate {
};

ImageScaler::ImageScaler() {
    p = new ImageScalerPrivate();
}

ImageScaler::~ImageScaler() {
    delete p;
}

bool ImageScaler::scale(const std::string &ifilename, const std::string &ofilename, ThumbnailSize wanted,
        const std::string &original_location, const std::string &rotation_source_file) const {
    const std::string &rotation_info = rotation_source_file.empty() ? ifilename : rotation_source_file;
    random_device rnd;
    assert(ifilename[0] == '/');
    assert(ofilename[0] == '/');
    GError *err = nullptr;
    string ofilename_tmp = ofilename;
    ofilename_tmp += ".tmp." + to_string(rnd());
    unique_gobj<GdkPixbuf> orig(gdk_pixbuf_new_from_file(ifilename.c_str(), &err));
    if(err) {
        string msg = err->message;
        g_error_free(err);
        throw runtime_error(msg);
    }
    unique_gobj<GdkPixbuf> src(fix_orientation(rotation_info.c_str(), orig.get()));
    if(!src) {
        throw runtime_error("Unknown error reorienting image.");
    }
    const int w = gdk_pixbuf_get_width(src.get());
    const int h = gdk_pixbuf_get_height(src.get());
    if(w == 0 || h == 0) {
        throw runtime_error("Invalid image resolution.");
    }

    int neww, newh;
    determine_new_size(w, h, neww, newh, wanted);
    unique_gobj<GdkPixbuf> dst(gdk_pixbuf_scale_simple(src.get(), neww, newh, GDK_INTERP_BILINEAR));
    gboolean save_ok;
    if(original_location.empty()) {
        save_ok = gdk_pixbuf_save(dst.get(), ofilename_tmp.c_str(), "png", &err, NULL);
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
        save_ok = gdk_pixbuf_save(dst.get(), ofilename_tmp.c_str(), "png", &err,
                "tEXt::Thumb::URI", uri.c_str(), "tEXt::Thumb::MTime", mtime_str.c_str(), NULL);
        if(!save_ok) {
            // Sigh. PNG text chunks are encoded in ISO-8859-1 but this is impossible if
            // the file name contains e.g. Chinese characters. Fall back to writing the
            // thumbnail without metadata. This does not conform to the thumbnailing standard
            // but at least it will give us a working thumbnail.
            g_error_free(err);
            err = nullptr;
            save_ok = gdk_pixbuf_save(dst.get(), ofilename_tmp.c_str(), "png", &err, NULL);
        }
    }
    if(!save_ok) {
        string msg = err->message;
        g_error_free(err);
        throw runtime_error(msg);
    }
    if(rename(ofilename_tmp.c_str(), ofilename.c_str()) <0) {
        string msg("Could not rename temp file to actual file: ");
        msg += strerror(errno);
        unlink(ofilename_tmp.c_str()); // Nothing we can do if it fails.
        throw runtime_error(msg);
    }
    return true;
}
