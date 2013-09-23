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

#ifndef THUMBNAILER_H_
#define THUMBNAILER_H_

class ThumbnailerPrivate;

enum ThumbnailSizes {
    TN_SIZE_SMALL,
    TN_SIZE_LARGE
};

/*
 * This class provides a way to generate and access
 * thumbnails of video, audio and image files.
 *
 * All methods are blocking.
 *
 * All methods are thread safe.
 *
 * Errors are reported as exceptions (exact types TBD).
 */

class Thumbnailer final {
public:
    Thumbnailer();
    ~Thumbnailer();

    Thumbnailer(const Thumbnailer &t) = delete;
    Thumbnailer & operator=(const Thumbnailer &t) = delete;

private:
    ThumbnailerPrivate *p;
};

#endif
