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

#include <internal/file_io.h>
#include <internal/image.h>

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
    string thumb;
    Image img;

    thumb = tn.get_thumbnail(TEST_IMAGE, QSize());
    img = Image(thumb);
    EXPECT_EQ(640, img.width());
    EXPECT_EQ(400, img.height());
    
    thumb = tn.get_thumbnail(TEST_IMAGE, QSize(160, 160));
    img = Image(thumb);
    EXPECT_EQ(160, img.width());
    EXPECT_EQ(100, img.height());

    thumb = tn.get_thumbnail(EXIF_IMAGE, QSize(1000, 1000));  // Will not up-scale
    img = Image(thumb);
    EXPECT_EQ(640, img.width());
    EXPECT_EQ(480, img.height());

    thumb = tn.get_thumbnail(EXIF_IMAGE, QSize(65, 65));  // From EXIF data
    img = Image(thumb);
    EXPECT_EQ(49, img.width());
    EXPECT_EQ(65, img.height());

    thumb = tn.get_thumbnail(RGB_IMAGE, QSize(48, 48));
    cout << "thumb size: " << thumb.size() << endl;
    write_file("/tmp/x.jpg", thumb);
    img = Image(thumb);
    EXPECT_EQ(48, img.width());
    EXPECT_EQ(48, img.height());

    string thumb2 = tn.get_thumbnail(RGB_IMAGE, QSize(48, 48));
    cout << "thumb2 size: " << thumb2.size() << endl;
    write_file("/tmp/y.jpg", thumb2);
    EXPECT_EQ(thumb, thumb2);
    img = Image(thumb2);
    EXPECT_EQ(48, img.width());
    EXPECT_EQ(48, img.height());
}
