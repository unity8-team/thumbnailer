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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <libexif/exif-loader.h>
#include <unity/util/ResourcePtr.h>

#include <string>

#include <unistd.h>

auto do_close = [](int fd)
{
    if (fd >= 0)
    {
        ::close(fd);
    }
};
typedef unity::util::ResourcePtr<int, decltype(do_close)> FdPtr;

auto do_unlink = [](std::string const& filename)
{
    ::unlink(filename.c_str());
};
typedef unity::util::ResourcePtr<std::string, decltype(do_unlink)> UnlinkPtr;

auto do_loader_close = [](GdkPixbufLoader* loader)
{
    if (loader)
    {
        gdk_pixbuf_loader_close(loader, NULL);
        g_object_unref(loader);
    }
};
typedef unity::util::ResourcePtr<GdkPixbufLoader*, decltype(do_loader_close)> LoaderPtr;

auto do_exif_loader_close = [](ExifLoader* loader)
{
    if (loader)
    {
        exif_loader_unref(loader);
    }
};
typedef unity::util::ResourcePtr<ExifLoader*, decltype(do_exif_loader_close)> ExifLoaderPtr;

auto do_exif_data_unref = [](ExifData* data)
{
    if (data)
    {
        exif_data_unref(data);
    }
};
typedef unity::util::ResourcePtr<ExifData*, decltype(do_exif_data_unref)> ExifDataPtr;
