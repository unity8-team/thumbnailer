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

#ifndef IMAGESCALER_H_
#define IMAGESCALER_H_

#include<string>

class ImageScalerPrivate;

class ImageScaler final {
public:
    ImageScaler();
    ~ImageScaler();

    ImageScaler(const ImageScaler &t) = delete;
    ImageScaler & operator=(const ImageScaler &t) = delete;

    bool scale(const std::string &ifilename,
            const std::string &ofilename,
            int wanted_size,

            // String pointing to the location of the original file,
            // as an example /home/someone/video.avi
            // We need this because we need to store its location
            // in the final image (rather than the location of the
            // temporary extracted image).
            const std::string &original_location,
            // What file to use for source of image rotation data.
            // Can not be the above file as it may be a non-image.
            // If empty, rotation info is taken from ifilename.
            const std::string &rotation_source_file="") const;

private:
    ImageScalerPrivate *p;
};


#endif
