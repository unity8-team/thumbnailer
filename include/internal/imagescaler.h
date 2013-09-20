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

#ifndef IMAGESCALER_H_
#define IMAGESCALER_H_

class ImageScalerPrivate;

class ImageScaler {
public:
    ImageScaler();
    ~ImageScaler();

    ImageScaler(const ImageScaler &t) = delete;
    ImageScaler & operator=(const ImageScaler &t) = delete;

private:
    ImageScalerPrivate *p;
};


#endif
