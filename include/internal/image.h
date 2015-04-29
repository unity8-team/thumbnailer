/*
 * Copyright (C) 2015 Canonical Ltd.
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
 * Authored by: MichiHenning <michi@canonical.com>
 */

#pragma once

#include <internal/gobj_memory.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <QSize>

#include <string>

class Image
{
public:
    // Default constructor does nothing. Must be followed by load() before calling any other member function.
    Image() = default;

    // Loads internal pixbuf with provided image data. The orientation
    // is as described by the EXIF specification. 1 means "already in correct orientation".
    Image(std::string const& data, QSize requested_size = QSize());

    Image(Image const&) = delete;
    Image& operator=(Image const&) = delete;
    Image(Image&&) = default;
    Image& operator=(Image &&) = default;

    int width() const;
    int height() const;

    // Return the pixel value at the (x,y) coordinates as an integer:
    //  r << 16 | g << 8 | b
    int pixel(int x, int y) const;

    // Returns image as JPEG data.
    std::string to_jpeg() const;


private:
    typedef unique_gobj<GdkPixbuf> PixbufPtr;
    PixbufPtr pixbuf_;
};
