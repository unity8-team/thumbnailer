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
#include <sys/types.h>
#include <fcntl.h>

#include <internal/file_io.h>
#include <internal/raii.h>
#include <testsetup.h>

#define TESTIMAGE TESTDATADIR "/orientation-1.jpg"
#define JPEGIMAGE TESTBINDIR "/saved_image.jpg"
#define BADIMAGE TESTDATADIR "/bad_image.jpg"
#define BIGIMAGE TESTDATADIR "/big.jpg"

using namespace std;
using namespace unity::thumbnailer::internal;

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
        EXPECT_EQ(0xFE0000, i.pixel(0, 0));
        EXPECT_EQ(0xFFFF00, i.pixel(639, 0));
        EXPECT_EQ(0x00FF01, i.pixel(639, 479));
        EXPECT_EQ(0x0000FE, i.pixel(0, 479));

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
}

TEST(Image, scale)
{
    string data = read_file(TESTIMAGE);
    Image img(data);
    EXPECT_EQ(640, img.width());
    EXPECT_EQ(480, img.height());

    Image scaled = img.scale(QSize(400, 400));
    EXPECT_EQ(400, scaled.width());
    EXPECT_EQ(300, scaled.height());

    // A large requested size results in no scaling
    scaled = img.scale(QSize(1000, 1000));
    EXPECT_EQ(640, scaled.width());
    EXPECT_EQ(480, scaled.height());

    // Aspect ratio maintained
    scaled = img.scale(QSize(1000, 240));
    EXPECT_EQ(320, scaled.width());
    EXPECT_EQ(240, scaled.height());

    // Scale to width
    scaled = img.scale(QSize(400, 0));
    EXPECT_EQ(400, scaled.width());
    EXPECT_EQ(300, scaled.height());

    // Scale to height
    scaled = img.scale(QSize(0, 300));
    EXPECT_EQ(400, scaled.width());
    EXPECT_EQ(300, scaled.height());
}

TEST(Image, save_jpeg)
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

TEST(Image, use_exif_thumbnail)
{
    string data = read_file(TESTIMAGE);
    Image img(data, QSize(160, 160));
    EXPECT_EQ(160, img.width());
    EXPECT_EQ(120, img.height());
    EXPECT_EQ(0xFE8081, img.pixel(0, 0));
    EXPECT_EQ(0xFFFF80, img.pixel(159, 0));
    EXPECT_EQ(0x81FF81, img.pixel(159, 119));
    EXPECT_EQ(0x807FFE, img.pixel(0, 119));
}

TEST(Image, orientation)
{
    for (int i = 1; i <= 8; i++) {
        auto filename = string(TESTDATADIR "/orientation-") + to_string(i) + ".jpg";
        string data = read_file(filename);
        Image img(data);
        EXPECT_EQ(640, img.width());
        EXPECT_EQ(480, img.height());
        EXPECT_EQ(0xFE0000, img.pixel(0, 0));
        EXPECT_EQ(0xFFFF00, img.pixel(639, 0));
        EXPECT_EQ(0x00FF01, img.pixel(639, 479));
        EXPECT_EQ(0x0000FE, img.pixel(0, 479));

        // Scaled version
        img = Image(data, QSize(320, 240));
        EXPECT_EQ(320, img.width());
        EXPECT_EQ(240, img.height());
        EXPECT_EQ(0xFE0000, img.pixel(0, 0));
        EXPECT_EQ(0xFFFF00, img.pixel(319, 0));
        EXPECT_EQ(0x00FF01, img.pixel(319, 239));
        EXPECT_EQ(0x0000FE, img.pixel(0, 239));

        // This version will be produced from the thumbnail, which has
        // been tinted to distinguish it from the original.
        img = Image(data, QSize(160, 160));
        EXPECT_EQ(160, img.width());
        EXPECT_EQ(120, img.height());
        EXPECT_EQ(0xFE8081, img.pixel(0, 0));
        EXPECT_EQ(0xFFFF80, img.pixel(159, 0));
        EXPECT_EQ(0x81FF81, img.pixel(159, 119));
        EXPECT_EQ(0x807FFE, img.pixel(0, 119));
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

TEST(Image, load_fd)
{
    FdPtr fd(open(TESTIMAGE, O_RDONLY), do_close);
    ASSERT_GT(fd.get(), 0);

    Image img(fd.get());
    EXPECT_EQ(640, img.width());
    EXPECT_EQ(480, img.height());
}

TEST(Image, load_fd_big_image)
{
    FdPtr fd(open(BIGIMAGE, O_RDONLY), do_close);
    ASSERT_GT(fd.get(), 0);

    // This image is significantly larger than the buffer used to read
    // the file, so multiple read() calls will be needed to fully
    // consume the image.
    Image img(fd.get());
    EXPECT_EQ(2731, img.width());
    EXPECT_EQ(2048, img.height());
}
