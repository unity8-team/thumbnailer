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

#include<string>

class Image
{
public:
    Image();
    Image(std::string const& data);

    Image(Image const&) = delete;
    Image& operator=(Image const&) = delete;
    Image(Image&&) = delete;
    Image& operator=(Image &&) = delete;

    int width() const;
    int height() const;
    int max_size() const;
    void scale_to(int desired_size);
    std::string get_jpeg() const;

private:
    unique_gobj<GdkPixbuf> pixbuf_;
};
