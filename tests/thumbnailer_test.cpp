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
 * Authored by: Michi Henning <michi.henning@canonical.com>
 */

#include <thumbnailer.h>

#include <testsetup.h>
#include <gtest/gtest.h>

#define TEST_IMAGE TESTDATADIR "/testimage.jpg"
#define PORTRAIT_IMAGE TESTDATADIR "/michi.jpg"
#define JPEG_IMAGE TESTBINDIR "/saved_image.jpg"
#define BAD_IMAGE TESTDATADIR "/bad_image.jpg"
#define RGB_IMAGE TESTDATADIR "/RGB.png"
#define EXIF_IMAGE TESTDATADIR "/testrotate.jpg"

using namespace std;

TEST(Thumbnailer, basic)
{
    Thumbnailer tn;

    string thumb = tn.get_thumbnail(TEST_IMAGE, 0);
    thumb = tn.get_thumbnail(TEST_IMAGE, 120);
    thumb = tn.get_thumbnail(EXIF_IMAGE, 1000);
    thumb = tn.get_thumbnail(EXIF_IMAGE, 60);
}
