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

#include <internal/thumbnailer.h>

#include <internal/file_io.h>
#include <internal/image.h>
#include <internal/raii.h>
#include <testsetup.h>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
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

// The thumbnailer uses g_get_user_cache_dir() to get the cache dir, and
// glib remembers that value, so changing XDG_CACHE_HOME later has no effect.

static auto set_tempdir = []()
{
    auto dir = new QTemporaryDir(TESTBINDIR "/thumbnailer-test.XXXXXX");
    setenv("XDG_CACHE_HOME", dir->path().toUtf8().data(), true);
    return dir;
};
static unique_ptr<QTemporaryDir> tempdir(set_tempdir());

class ThumbnailerTest : public ::testing::Test
{
protected:
    virtual void SetUp() override
    {
        mkdir(tempdir_path().c_str(), 0700);
    }

    virtual void TearDown() override
    {
        boost::filesystem::remove_all(tempdir_path());
    }

    static string tempdir_path()
    {
        return tempdir->path().toUtf8().data();
    }
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

    // Again, this time we get the answer from the failure cache.
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

TEST_F(ThumbnailerTest, bad_fd)
{
    Thumbnailer tn;

    // Invalid file descriptor
    try
    {
        tn.get_thumbnail(TEST_IMAGE, -1, QSize());
    }
    catch (std::exception const& e)
    {
        string msg = e.what();
        EXPECT_TRUE(boost::contains(msg, ": Could not stat file descriptor:")) << msg;
    }

    // File descriptor for wrong file
    FdPtr fd(open(TEST_VIDEO, O_RDONLY), do_close);
    try
    {
        tn.get_thumbnail(TEST_IMAGE, fd.get(), QSize());
    }
    catch (std::exception const& e)
    {
        string msg = e.what();
        EXPECT_TRUE(boost::contains(msg, ": file descriptor does not refer to file ")) << msg;
    }
}

TEST_F(ThumbnailerTest, replace_photo)
{
    string testfile = tempdir_path() + "/foo.jpg";
    ASSERT_EQ(0, link(TEST_IMAGE, testfile.c_str()));

    Thumbnailer tn;
    FdPtr fd(open(testfile.c_str(), O_RDONLY), do_close);
    auto request = tn.get_thumbnail(testfile, fd.get(), QSize());
    // The client FD isn't needed any more, so close it.
    fd.reset(-1);

    // Replace test image with a different file with different
    // dimensions so we can tell which one is thumbnailed.
    ASSERT_EQ(0, unlink(testfile.c_str()));
    ASSERT_EQ(0, link(BIG_IMAGE, testfile.c_str()));

    string data = request->thumbnail();
    Image img(data);
    EXPECT_EQ(640, img.width());
    EXPECT_EQ(480, img.height());
}

TEST_F(ThumbnailerTest, thumbnail_video)
{
    Thumbnailer tn;
    FdPtr fd(open(TEST_VIDEO, O_RDONLY), do_close);
    auto request = tn.get_thumbnail(TEST_VIDEO, fd.get(), QSize());
    ASSERT_NE(nullptr, request.get());
    // Video thumbnails cannot be produced immediately
    ASSERT_EQ("", request->thumbnail());

    QSignalSpy spy(request.get(), &ThumbnailRequest::downloadFinished);
    request->download(chrono::milliseconds(15000));
    ASSERT_TRUE(spy.wait(20000));
    {
        string thumb = request->thumbnail();
        ASSERT_NE("", thumb);
        Image img(thumb);
        EXPECT_EQ(1920, img.width());
        EXPECT_EQ(1080, img.height());
    }

    {
        // Fetch the thumbnail again with the same size.
        // That causes it to come from the thumbnail cache.
        auto request = tn.get_thumbnail(TEST_VIDEO, fd.get(), QSize());
        string thumb = request->thumbnail();
        ASSERT_NE("", thumb);
        Image img(thumb);
        EXPECT_EQ(1920, img.width());
        EXPECT_EQ(1080, img.height());
    }

    {
        // Fetch the thumbnail again with a different size.
        // That causes it to be scaled from the thumbnail cache.
        auto request = tn.get_thumbnail(TEST_VIDEO, fd.get(), QSize(500, 500));
        string thumb = request->thumbnail();
        ASSERT_NE("", thumb);
        Image img(thumb);
        EXPECT_EQ(500, img.width());
        EXPECT_EQ(281, img.height());
    }
}

TEST_F(ThumbnailerTest, replace_video)
{
    string testfile = tempdir_path() + "/foo.ogv";
    ASSERT_EQ(0, link(TEST_VIDEO, testfile.c_str())) << strerror(errno);

    Thumbnailer tn;
    FdPtr fd(open(testfile.c_str(), O_RDONLY | O_CLOEXEC), do_close);
    auto request = tn.get_thumbnail(testfile, fd.get(), QSize());
    // The client FD isn't needed any more, so close it.
    fd.reset(-1);

    // Replace test image with a different file with different
    // dimensions so we can tell which one is thumbnailed.
    ASSERT_EQ(0, unlink(testfile.c_str()));
    ASSERT_EQ(0, link(BIG_IMAGE, testfile.c_str()));

    ASSERT_EQ("", request->thumbnail());
    QSignalSpy spy(request.get(), &ThumbnailRequest::downloadFinished);
    request->download(chrono::milliseconds(15000));
    ASSERT_TRUE(spy.wait(20000));

    string data = request->thumbnail();
    Image img(data);
    EXPECT_EQ(1920, img.width());
    EXPECT_EQ(1080, img.height());
}

TEST_F(ThumbnailerTest, thumbnail_song)
{
    Thumbnailer tn;
    FdPtr fd(open(TEST_SONG, O_RDONLY), do_close);
    auto request = tn.get_thumbnail(TEST_SONG, fd.get(), QSize());
    ASSERT_NE(nullptr, request.get());
    // Audio thumbnails cannot be produced immediately
    ASSERT_EQ("", request->thumbnail());

    QSignalSpy spy(request.get(), &ThumbnailRequest::downloadFinished);
    request->download(chrono::milliseconds(15000));
    ASSERT_TRUE(spy.wait(20000));
    string thumb = request->thumbnail();
    ASSERT_NE("", thumb);
    Image img(thumb);
    EXPECT_EQ(200, img.width());
    EXPECT_EQ(200, img.height());
}

TEST_F(ThumbnailerTest, exceptions)
{
    string const cache_dir = tempdir_path();
    ASSERT_EQ(0, chmod(cache_dir.c_str(), 0000));
    try
    {
        Thumbnailer tn;
        FAIL();
    }
    catch (runtime_error const& e)
    {
        ASSERT_EQ(0, chmod(cache_dir.c_str(), 0700));
        string msg = e.what();
        string exp = "Thumbnailer(): Cannot instantiate cache: PersistentStringCache: cannot open or create cache: ";
        EXPECT_EQ(0, msg.compare(0, exp.length(), exp)) << msg;
    }
    ASSERT_EQ(0, chmod(cache_dir.c_str(), 0700));
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    setenv("TN_UTILDIR", TESTBINDIR "/../src/vs-thumb", true);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
