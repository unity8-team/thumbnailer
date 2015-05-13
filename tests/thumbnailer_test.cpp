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
#include <internal/raii.h>
#include <testsetup.h>

#include <boost/algorithm/string.hpp>
#include <gtest/gtest.h>
#include <QCoreApplication>
#include <QSignalSpy>
#include <QTemporaryDir>

#include <sys/types.h>
#include <fcntl.h>

#define TEST_IMAGE TESTDATADIR "/orientation-1.jpg"
#define BAD_IMAGE TESTDATADIR "/bad_image.jpg"
#define RGB_IMAGE TESTDATADIR "/RGB.png"
#define BIG_IMAGE TESTDATADIR "/big.jpg"
#define EMPTY_IMAGE TESTDATADIR "/empty"

#define TEST_VIDEO TESTDATADIR "/testvideo.ogg"
#define TEST_SONG TESTDATADIR "/testsong.ogg"

using namespace std;
using namespace unity::thumbnailer::internal;

class ThumbnailerTest : public ::testing::Test
{
protected:
    virtual void SetUp() override {
        tempdir.reset(new QTemporaryDir(TESTBINDIR "/thumbnailer-test.XXXXXX"));
        ASSERT_TRUE(tempdir->isValid());
        setenv("XDG_CACHE_HOME", tempdir->path().toUtf8().data(), true);
    }

    string tempdir_path() const {
        return tempdir->path().toStdString();
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
    FdPtr fd(-1, do_close);

    fd.reset(open(EMPTY_IMAGE, O_RDONLY));
    thumb = tn.get_thumbnail(EMPTY_IMAGE, fd.get(), QSize())->thumbnail();
    EXPECT_EQ("", thumb);

    fd.reset(open(TEST_IMAGE, O_RDONLY));
    thumb = tn.get_thumbnail(TEST_IMAGE, fd.get(), QSize())->thumbnail();
    img = Image(thumb);
    EXPECT_EQ(640, img.width());
    EXPECT_EQ(480, img.height());
    
    // Again, for coverage. This time the thumbnail comes from the cache.
    thumb = tn.get_thumbnail(TEST_IMAGE, fd.get(), QSize())->thumbnail();
    img = Image(thumb);
    EXPECT_EQ(640, img.width());
    EXPECT_EQ(480, img.height());

    thumb = tn.get_thumbnail(TEST_IMAGE, fd.get(), QSize(160, 160))->thumbnail();
    img = Image(thumb);
    EXPECT_EQ(160, img.width());
    EXPECT_EQ(120, img.height());

    thumb = tn.get_thumbnail(TEST_IMAGE, fd.get(), QSize(1000, 1000))->thumbnail();  // Will not up-scale
    img = Image(thumb);
    EXPECT_EQ(640, img.width());
    EXPECT_EQ(480, img.height());

    thumb = tn.get_thumbnail(TEST_IMAGE, fd.get(), QSize(100, 100))->thumbnail();  // From EXIF data
    img = Image(thumb);
    EXPECT_EQ(100, img.width());
    EXPECT_EQ(75, img.height());

    fd.reset(open(BAD_IMAGE, O_RDONLY));
    try
    {
        tn.get_thumbnail(BAD_IMAGE, fd.get(), QSize())->thumbnail();
    }
    catch (std::exception const& e)
    {
        string msg = e.what();
        EXPECT_TRUE(boost::starts_with(msg, "load_image(): cannot close pixbuf loader: ")) << msg;
    }

    fd.reset(open(RGB_IMAGE, O_RDONLY));
    thumb = tn.get_thumbnail(RGB_IMAGE, fd.get(), QSize(48, 48))->thumbnail();
    cout << "thumb size: " << thumb.size() << endl;
    img = Image(thumb);
    EXPECT_EQ(48, img.width());
    EXPECT_EQ(48, img.height());

    fd.reset(open(BIG_IMAGE, O_RDONLY));
    thumb = tn.get_thumbnail(BIG_IMAGE, fd.get(), QSize())->thumbnail();  // > 1920, so will be trimmed down
    img = Image(thumb);
    EXPECT_EQ(1920, img.width());
    EXPECT_EQ(1439, img.height());

    thumb = tn.get_thumbnail(BIG_IMAGE, fd.get(), QSize(0, 0))->thumbnail();  // unconstrained, so will not be trimmed down
    img = Image(thumb);
    EXPECT_EQ(2731, img.width());
    EXPECT_EQ(2048, img.height());
}

TEST_F(ThumbnailerTest, thumbnail_video)
{
    Thumbnailer tn;
    FdPtr fd(open(TEST_VIDEO, O_RDONLY), do_close);
    auto request = tn.get_thumbnail(TEST_VIDEO, fd.get(), QSize());
    ASSERT_NE(nullptr, request.get());
    // Video thumbnails can not be produced immediately
    ASSERT_EQ("", request->thumbnail());

    QSignalSpy spy(request.get(), &ThumbnailRequest::downloadFinished);
    request->download();
    ASSERT_TRUE(spy.wait(20000));
    string thumb = request->thumbnail();
    ASSERT_NE("", thumb);
    Image img(thumb);
    EXPECT_EQ(1920, img.width());
    EXPECT_EQ(1080, img.height());
}

TEST_F(ThumbnailerTest, thumbnail_song)
{
    Thumbnailer tn;
    FdPtr fd(open(TEST_SONG, O_RDONLY), do_close);
    auto request = tn.get_thumbnail(TEST_SONG, fd.get(), QSize());
    ASSERT_NE(nullptr, request.get());
    // Audio thumbnails can not be produced immediately
    ASSERT_EQ("", request->thumbnail());

    QSignalSpy spy(request.get(), &ThumbnailRequest::downloadFinished);
    request->download();
    ASSERT_TRUE(spy.wait(20000));
    string thumb = request->thumbnail();
    ASSERT_NE("", thumb);
    Image img(thumb);
    EXPECT_EQ(200, img.width());
    EXPECT_EQ(200, img.height());
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    setenv("TN_UTILDIR", TESTBINDIR "/../src/vs-thumb", true);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
