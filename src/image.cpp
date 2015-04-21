/*
 * Copyright (C) 2015 Canonical Ltd
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
 * Authored by: Michi Henning <michi.henning@canonical.com>
 */

#include <internal/image.h>

#include <internal/raii.h>

#include <cassert>
#include <cmath>

using namespace unity::thumbnailer::internal;
using namespace std;

Image::Image(string const& data, int orientation)
{
    load(data, orientation);
}

void Image::load(string const& data, int orientation)
{
    LoaderPtr loader(gdk_pixbuf_loader_new(), do_loader_close);
    if (!loader.get())
    {
        throw runtime_error("Image::load(): cannot allocate GdkPixbufLoader");  // LCOV_EXCL_LINE
    }
    GError* err = nullptr;
    if (!gdk_pixbuf_loader_write(loader.get(), reinterpret_cast<guchar const*>(&data[0]), data.size(), &err))
    {
        // LCOV_EXCL_START
        string msg = string("Image::load): cannot write to pixbuf loader: ") + err->message;
        g_error_free(err);
        throw runtime_error(msg);
        // LCOV_EXCL_STOP
    }
    pixbuf_.reset(gdk_pixbuf_loader_get_pixbuf(loader.get()));
    if (!pixbuf_)
    {
        throw runtime_error("Image::load(): cannot create pixbuf");
    }
    g_object_ref(pixbuf_.get());  // Closing the loader calls unref on the pixbuf, so we need to increment here.

    fix_orientation(orientation);
}

int Image::width() const
{
    assert(pixbuf_);

    auto w = gdk_pixbuf_get_width(pixbuf_.get());
    if (w < 1)
    {
        throw runtime_error("Image::width(): invalid image width: " + to_string(w));  // LCOV_EXCL_LINE
    }
    return w;
}

int Image::height() const
{
    assert(pixbuf_);

    auto h = gdk_pixbuf_get_height(pixbuf_.get());
    if (h < 1)
    {
        throw runtime_error("Image::height(): invalid image height: " + to_string(h));  // LCOV_EXCL_LINE
    }
    return h;
}

int Image::max_size() const
{
    return max(width(), height());
}

void Image::scale_to(int desired_size)
{
    assert(pixbuf_);
    assert(desired_size > 0);

    int const w = width();
    int const h = height();

    int new_w;
    int new_h;
    if (w > h)
    {
        new_w = desired_size;
        new_h = round(desired_size * (double(h) / w));
    }
    else
    {
        new_w = round(desired_size * (double(w) / h));
        new_h = desired_size;
    }
    // Make sure we retain at least 1 pixel in each dimension.
    new_w = max(new_w, 1);
    new_h = max(new_h, 1);

    auto p = gdk_pixbuf_scale_simple(pixbuf_.get(), new_w, new_h, GDK_INTERP_BILINEAR);
    if (!p)
    {
        // LCOV_EXCL_START
        throw runtime_error("Image::scale(): cannot scale to " + to_string(new_w) + "x" + to_string(new_h));
        // LCOV_EXCL_STOP
    }
    pixbuf_.reset(p);
}

string Image::to_jpeg() const
{
    assert(pixbuf_);

    gchar* buf;
    gsize size;
    GError* err = nullptr;
    if (!gdk_pixbuf_save_to_buffer(pixbuf_.get(), &buf, &size, "jpeg", &err, "quality", "100", NULL))
    {
        // LCOV_EXCL_START
        string msg = string("Image::get_data(): cannot convert to jpeg: ") + err->message;
        g_error_free(err);
        throw runtime_error(msg);
        // LCOV_EXCL_STOP
    }
    string s(buf, size);
    g_free(buf);
    return s;
}

void Image::fix_orientation(int orientation)
{
    // Doing this manually instead of calling gdk_pixbuf_apply_embedded_orientation() because of
    // https://bugzilla.gnome.org/show_bug.cgi?id=725582
    switch (orientation)
    {
        case 1:
        {
            break;  // Already in correct orientation
        }
        case 2:
        {
            // Horizontal mirror image
            pixbuf_.reset(gdk_pixbuf_flip(pixbuf_.get(), true));
            break;
        }
        case 3:
        {
            // Rotate 180
            pixbuf_.reset(gdk_pixbuf_rotate_simple(pixbuf_.get(), GDK_PIXBUF_ROTATE_UPSIDEDOWN));
            break;
        }
        case 4:
        {
            // Vertical mirror image.
            pixbuf_.reset(gdk_pixbuf_flip(pixbuf_.get(), false));
            break;
        }
        case 5:
        {
            // Rotate 90 clockwise and horizontal mirror image
            pixbuf_.reset(gdk_pixbuf_rotate_simple(pixbuf_.get(), GDK_PIXBUF_ROTATE_CLOCKWISE));
            pixbuf_.reset(gdk_pixbuf_flip(pixbuf_.get(), true));
            break;
        }
        case 6:
        {
            // Rotate 90 clockwise
            pixbuf_.reset(gdk_pixbuf_rotate_simple(pixbuf_.get(), GDK_PIXBUF_ROTATE_CLOCKWISE));
            break;
        }
        case 7:
        {
            // Rotate 90 anti-clockwise and horizontal mirror image
            pixbuf_.reset(gdk_pixbuf_rotate_simple(pixbuf_.get(), GDK_PIXBUF_ROTATE_COUNTERCLOCKWISE));
            pixbuf_.reset(gdk_pixbuf_flip(pixbuf_.get(), true));
            break;
        }
        case 8:
        {
            // Rotate 90 anti-clockwise
            pixbuf_.reset(gdk_pixbuf_rotate_simple(pixbuf_.get(), GDK_PIXBUF_ROTATE_COUNTERCLOCKWISE));
            break;
        }
        default:
        {
            // Impossible, according the spec. Rather than throwing or some such,
            // we do nothing and return the EXIF image without any adjustment.
            break;
        }
    }
}
