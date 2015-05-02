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

#include <boost/algorithm/string.hpp>
#include <gtest/gtest.h>
#include <QCoreApplication>
#include <QTemporaryDir>

#define TEST_IMAGE TESTDATADIR "/orientation-1.jpg"
#define BAD_IMAGE TESTDATADIR "/bad_image.jpg"
#define RGB_IMAGE TESTDATADIR "/RGB.png"
#define BIG_IMAGE TESTDATADIR "/big.jpg"

using namespace std;

class ThumbnailerTest : public ::testing::Test
{
protected:
    virtual void SetUp() override {
        tempdir.reset(new QTemporaryDir(TESTBINDIR "/thumbnailer-test.XXXXXX"));
        ASSERT_TRUE(tempdir->isValid());
        setenv("XDG_CACHE_HOME", tempdir->path().toUtf8().data(), true);
    }

    virtual void TearDown() override {
        tempdir.reset();
    }
    unique_ptr<QTemporaryDir> tempdir;
};


TEST_F(ThumbnailerTest, basic)
{
    Thumbnailer tn;
    string thumb;
    Image img;

    thumb = tn.get_thumbnail(TEST_IMAGE, QSize());
    img = Image(thumb);
    EXPECT_EQ(640, img.width());
    EXPECT_EQ(480, img.height());

    thumb = tn.get_thumbnail(TEST_IMAGE, QSize(160, 160));
    img = Image(thumb);
    EXPECT_EQ(160, img.width());
    EXPECT_EQ(120, img.height());

    thumb = tn.get_thumbnail(TEST_IMAGE, QSize(1000, 1000));  // Will not up-scale
    img = Image(thumb);
    EXPECT_EQ(640, img.width());
    EXPECT_EQ(480, img.height());

    thumb = tn.get_thumbnail(TEST_IMAGE, QSize(100, 100));  // From EXIF data
    img = Image(thumb);
    EXPECT_EQ(100, img.width());
    EXPECT_EQ(75, img.height());

    try
    {
        tn.get_thumbnail(BAD_IMAGE, QSize());
    }
    catch (std::exception const& e)
    {
        string msg = e.what();
        EXPECT_TRUE(boost::starts_with(msg, "load_image(): cannot close pixbuf loader: ")) << msg;
    }

    thumb = tn.get_thumbnail(RGB_IMAGE, QSize(48, 48));
    img = Image(thumb);
    EXPECT_EQ(48, img.width());
    EXPECT_EQ(48, img.height());

    thumb = tn.get_thumbnail(BIG_IMAGE, QSize());  // > 1920, so will be trimmed down
    img = Image(thumb);
    EXPECT_EQ(1920, img.width());
    EXPECT_EQ(1439, img.height());

    thumb = tn.get_thumbnail(BIG_IMAGE, QSize(0, 0));  // unconstrained, so will not be trimmed down
    img = Image(thumb);
    EXPECT_EQ(2731, img.width());
    EXPECT_EQ(2048, img.height());
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
