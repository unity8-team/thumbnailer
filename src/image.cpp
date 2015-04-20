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

#include <unity/util/ResourcePtr.h>

#include <cassert>
#include <cmath>

using namespace std;

namespace
{

auto do_loader_close = [](GdkPixbufLoader* loader)
{
    if (loader)
    {
        gdk_pixbuf_loader_close(loader, NULL);
        g_object_unref(loader);
    }
};
typedef unity::util::ResourcePtr<GdkPixbufLoader*, decltype(do_loader_close)> LoaderPtr;

}  // namespace

Image::Image() = default;

Image::Image(string const& data)
{
    load(data);
}

void Image::load(string const& data)
{
    LoaderPtr loader(gdk_pixbuf_loader_new(), do_loader_close);
    if (!loader)
    {
        throw runtime_error("Image::load(): cannot allocate GdkPixbufLoader");
    }
    GError *err = nullptr;
    if (!gdk_pixbuf_loader_write(loader.get(), reinterpret_cast<guchar const*>(&data[0]), data.size(), &err))
    {
        string msg = string("Image::load): cannot write to pixbuf loader: ") + err->message;
        g_error_free(err);
        throw runtime_error(msg);
    }
    pixbuf_.reset(gdk_pixbuf_loader_get_pixbuf(loader.get()));
    if (!pixbuf_)
    {
        throw runtime_error("Image::load(): cannot create pixbuf");
    }
    g_object_ref(pixbuf_.get());  // Closing the loader calls unref on the pixbuf, so we need to increment here.
}

int Image::width() const
{
    assert(pixbuf_);

    auto w = gdk_pixbuf_get_width(pixbuf_.get());
    if (w < 1)
    {
        throw runtime_error("Image::width(): invalid image width: " + to_string(w));
    }
    return w;
}

int Image::height() const
{
    assert(pixbuf_);

    auto h = gdk_pixbuf_get_height(pixbuf_.get());
    if (h < 1)
    {
        throw runtime_error("Image::height(): invalid image height: " + to_string(h));
    }
    return h;
}

int Image::max_size() const
{
    return max(width(), height());
}

void Image::scale_to(int desired_size)
{
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
        throw runtime_error("Image::scale(): cannot scale to " + to_string(new_w) + "x" + to_string(new_h));
    }
    pixbuf_.reset(p);
}

string Image::get_jpeg() const
{
    assert(pixbuf_);

    gchar* buf = nullptr;
    gsize size;
    GError *err = nullptr;
    if (!gdk_pixbuf_save_to_buffer(pixbuf_.get(), &buf, &size, "jpeg", &err, "quality", "100", NULL))
    {
        string msg = string("Image::get_data(): cannot convert to jpeg: ") + err->message;
        g_error_free(err);
        throw runtime_error(msg);
    }
    return string(buf, size);
}
