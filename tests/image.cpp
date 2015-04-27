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

#include <internal/image.h>

#include <boost/algorithm/string.hpp>
#include <gtest/gtest.h>
#include <internal/file_io.h>
#include <testsetup.h>

#define TESTIMAGE TESTDATADIR "/testimage.jpg"
#define PORTRAITIMAGE TESTDATADIR "/michi.jpg"
#define JPEGIMAGE TESTBINDIR "/saved_image.jpg"
#define BADIMAGE TESTDATADIR "/bad_image.jpg"
#define RGBIMAGE TESTDATADIR "/RGB.png"

using namespace std;

TEST(Image, basic)
{
    {
        Image i;
    }

    {
        string data = read_file(TESTIMAGE);
        Image i(data);
        EXPECT_EQ(640, i.width());
        EXPECT_EQ(400, i.height());
        EXPECT_EQ(640, i.max_size());

        // Move constructor
        Image i2(move(i));
        EXPECT_EQ(640, i2.width());
        EXPECT_EQ(400, i2.height());

        // Move assignment
        Image i3;
        i3 = move(i2);
        EXPECT_EQ(640, i3.width());
        EXPECT_EQ(400, i3.height());

        // Down-scale
        i3.scale_to(320);
        EXPECT_EQ(320, i3.width());
        EXPECT_EQ(200, i3.height());

        // Up-scale
        i3.scale_to(960);
        EXPECT_EQ(960, i3.width());
        EXPECT_EQ(600, i3.height());

        // Test that we can't go below 1x1.
        i3.scale_to(1);
        EXPECT_EQ(1, i3.width());
        EXPECT_EQ(1, i3.height());
    }

    {
        string data = read_file(PORTRAITIMAGE);
        Image i(data);
        EXPECT_EQ(39, i.width());
        EXPECT_EQ(48, i.height());
        EXPECT_EQ(48, i.max_size());

        // Up-scale
        i.scale_to(100);
        EXPECT_EQ(81, i.width());
        EXPECT_EQ(100, i.height());

        // Down-scale
        i.scale_to(20);
        EXPECT_EQ(16, i.width());
        EXPECT_EQ(20, i.height());
    }

    {
        string data = read_file(TESTIMAGE);
        Image i(data);
        EXPECT_EQ(640, i.width());
        EXPECT_EQ(400, i.height());

        string jpeg = i.to_jpeg();
        write_file(JPEGIMAGE, jpeg);
        // No test here. Because JPEG is lossy, there is no easy way to verify
        // that the image was saved correctly. Manual inspection of the file
        // is easier (see JPEGIMAGE in the test build dir).
    }
}

TEST(Image, rotate)
{
    string data = read_file(RGBIMAGE);

    for (int i = 0; i <= 9; ++i)
    {
        Image img(data, i);
        // TODO: No test here yet, this is for coverage.
        //       Need to add pixel sampling functionality to Image.
    }
}

TEST(Image, exceptions)
{
    {
        try
        {
            string data = read_file(BADIMAGE);
            Image i(data);
            FAIL();
        }
        catch (std::exception const& e)
        {
            string msg = e.what();
            EXPECT_TRUE(boost::starts_with(msg, "Image::load): cannot close pixbuf loader: ")) << msg;
        }
    }
}
