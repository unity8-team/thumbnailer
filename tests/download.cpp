/*
 * Copyright (C) 2014 Canonical Ltd.
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
 * Authored by: Jussi Pakkanen <jussi.pakkanen@canonical.com>
 */

#include<gtest/gtest.h>
#include<internal/lastfmdownloader.h>
#include<cstdio>

using namespace std;

const char *testimage = "abc";

class FakeDownloader : public HttpDownloader {
private:
    constexpr static const char *imloc = "http://dummy";
    constexpr static const char *xml = "<album><coverart><large>http://dummy</large></coverart></album>";

public:
    std::string download(const std::string &url) override {
        if(url.find("audioscrobbler") != std::string::npos) {
            return xml;
        }
        if(url == imloc) {
            return testimage;
        }
        throw std::runtime_error("Tried to get unknown data from FakeDownloader.");
    }
};

TEST(Downloader, canned) {
    LastFMDownloader lfdl(new FakeDownloader());
    string artist("Some guy");
    string album("Some album");
    string outfile("/tmp/temptestfile");
    ASSERT_TRUE(lfdl.download(artist, album, outfile));
    char output[4];
    FILE *f = fopen(outfile.c_str(), "r");
    fread(output, 3, 1, f);
    output[3] = '\0';
    fclose(f);
    string s1(testimage);
    string s2(output);
    ASSERT_EQ(s1, s2);
}


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
