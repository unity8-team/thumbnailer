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
 * Authored by: James Henstridge <james.henstridge@canonical.com>
 */

#include <cstdlib>
#include <memory>
#include <stdexcept>
#include <string>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gio/gio.h>
#include <gst/gst.h>
#include <gtest/gtest.h>

#include <testsetup.h>
#include <internal/gobj_memory.h>
#include "../src/vs-thumb/thumbnailextractor.h"

const char THEORA_TEST_FILE[] = TESTDATADIR "/testvideo.ogg";

class ExtractorTest : public ::testing::Test {
protected:
    ExtractorTest() {}
    virtual ~ExtractorTest() {}

    virtual void SetUp() override {
        tempdir = "./vsthumb-test.XXXXXX";
        if (mkdtemp(const_cast<char*>(tempdir.data())) == nullptr) {
            tempdir = "";
            throw std::runtime_error("Could not create temporary directory");
        }
    }

    virtual void TearDown() override {
        if (!tempdir.empty()) {
            std::string cmd = "rm -rf \"" + tempdir + "\"";
            ASSERT_EQ(system(cmd.c_str()), 0);
        }
    }

    std::string tempdir;
};

std::string filename_to_uri(const std::string &filename) {
    std::unique_ptr<GFile, void(*)(void *)> file(
        g_file_new_for_path(filename.c_str()), g_object_unref);
    if (!file) {
        throw std::runtime_error("Could not create GFile");
    }
    std::unique_ptr<char, void(*)(void*)> uri(
        g_file_get_uri(file.get()), g_free);
    return uri.get();
}

TEST_F(ExtractorTest, extract_theora) {
    ThumbnailExtractor extractor;
    std::string outfile = tempdir + "/out.jpg";

    extractor.set_uri(filename_to_uri(THEORA_TEST_FILE));
    extractor.extract_frame();
    extractor.save_screenshot(outfile);

    GError *error = nullptr;
    unique_gobj<GdkPixbuf> image(
        gdk_pixbuf_new_from_file(outfile.c_str(), &error));
    if (error) {
        EXPECT_EQ(error, nullptr) << error->message;
        g_error_free(error);
        return;
    }

    EXPECT_EQ(gdk_pixbuf_get_width(image.get()), 1920);
    EXPECT_EQ(gdk_pixbuf_get_height(image.get()), 1080);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    gst_init(&argc, &argv);
    return RUN_ALL_TESTS();
}
