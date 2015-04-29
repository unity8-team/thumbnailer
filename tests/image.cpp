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

#define TESTIMAGE TESTDATADIR "/orientation-1.jpg"
#define JPEGIMAGE TESTBINDIR "/saved_image.jpg"
#define BADIMAGE TESTDATADIR "/bad_image.jpg"

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
        EXPECT_EQ(480, i.height());

        // Move constructor
        Image i2(move(i));
        EXPECT_EQ(640, i2.width());
        EXPECT_EQ(480, i2.height());

        // Move assignment
        Image i3;
        i3 = move(i2);
        EXPECT_EQ(640, i3.width());
        EXPECT_EQ(480, i3.height());

        // Load to fit in bounding box
        Image i4(data, QSize(320, 320));
        EXPECT_EQ(320, i4.width());
        EXPECT_EQ(240, i4.height());

        // Load to fit width
        Image i5(data, QSize(320, 0));
        EXPECT_EQ(320, i5.width());
        EXPECT_EQ(240, i5.height());

        // Load to fit height
        Image i6(data, QSize(0, 240));
        EXPECT_EQ(320, i6.width());
        EXPECT_EQ(240, i6.height());
    }

    {
        string data = read_file(TESTIMAGE);
        Image i(data);
        EXPECT_EQ(640, i.width());
        EXPECT_EQ(480, i.height());

        string jpeg = i.to_jpeg();
        Image i2(jpeg);
        EXPECT_EQ(640, i2.width());
        EXPECT_EQ(480, i2.height());
        // No test here. Because JPEG is lossy, there is no easy way to verify
        // that the image was saved correctly. Manual inspection of the file
        // is easier (see JPEGIMAGE in the test build dir).
        write_file(JPEGIMAGE, jpeg);
    }
}

TEST(Image, orientation)
{
    for (int i = 1; i <= 8; i++) {
        auto filename = string(TESTDATADIR "/orientation-") + to_string(i) + ".jpg";
        string data = read_file(filename);
        Image img(data);
        EXPECT_EQ(640, img.width());
        EXPECT_EQ(480, img.height());
        // Check corners

        img = Image(data, QSize(160, 160));
        EXPECT_EQ(160, img.width());
        EXPECT_EQ(120, img.height());
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
            EXPECT_TRUE(boost::starts_with(msg, "load_image(): cannot close pixbuf loader: ")) << msg;
        }
    }
}
