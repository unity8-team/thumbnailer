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

#ifndef VIDEOSCREENSHOTTER_H_
#define VIDEOSCREENSHOTTER_H_

class VideoScreenshotterPrivate;

class VideoScreenshotter {
public:
    VideoScreenshotter();
    ~VideoScreenshotter();

    VideoScreenshotter(const VideoScreenshotter &t) = delete;
    VideoScreenshotter & operator=(const VideoScreenshotter &t) = delete;

private:
    VideoScreenshotterPrivate *p;
};


#endif
