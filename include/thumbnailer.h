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

#ifndef THUMBNAILER_H_
#define THUMBNAILER_H_

#include<string>

#if defined _WIN32 || defined __CYGWIN__
  #ifdef BUILDING_THUMBNAILER
    #define LTN_PUBLIC __declspec(dllexport)
  #else
    #define LTN_PUBLIC __declspec(dllimport)
  #endif
#else
  #if defined __GNUC__
    #define LTN_PUBLIC __attribute__ ((visibility("default")))
  #else
    #pragma message ("Compiler does not support symbol visibility.")
    #define LTN_PUBLIC
  #endif
#endif


class ThumbnailerPrivate;

enum ThumbnailSize {
    TN_SIZE_SMALL,   // maximum dimension 128 pixels
    TN_SIZE_LARGE,   // maximum dimension 256 pixels
    TN_SIZE_XLARGE,  // maximum dimension 512 pixels
    TN_SIZE_ORIGINAL // Whatever the original size was, e.g. 1920x1080 for FullHD video
};

/**
 * This class provides a way to generate and access
 * thumbnails of video, audio and image files.
 *
 * All methods are blocking.
 *
 * All methods are thread safe.
 *
 * Errors are reported as exceptions.
 */

class Thumbnailer {
public:
    Thumbnailer();
    ~Thumbnailer();

    /**
     * Gets a thumbnail of the given input file in the requested size.
     *
     * Return value is a string pointing to the thumbnail file. If
     * the thumbnail could not be generated and empty string is returned.
     *
     * Applications should treat the returned file as read only. They should _not_
     * delete it.
     *
     * In case of unexpected problems, the function throws a
     * std::runtime_error.
     */
    std::string get_thumbnail(const std::string &filename, ThumbnailSize desired_size);

private:
    Thumbnailer(const Thumbnailer &t);
    Thumbnailer & operator=(const Thumbnailer &t);

    ThumbnailerPrivate *p;
};

#endif
