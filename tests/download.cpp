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
#include<unistd.h>
#include<mutex>
#include<condition_variable>
#include<thread>

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
    unlink(outfile.c_str());
    ASSERT_TRUE(lfdl.download(artist, album, outfile));
    char output[4];
    FILE *f = fopen(outfile.c_str(), "r");
    ASSERT_TRUE(f);
    ASSERT_EQ(fread(output, 1, 3, f), 3);
    output[3] = '\0';
    fclose(f);
    unlink(outfile.c_str());
    string s1(testimage);
    string s2(output);
    ASSERT_EQ(s1, s2);
}

static std::mutex m;
static std::condition_variable cv;
static bool go = false;

static void query_thread(LastFMDownloader *lfdl, int num) {
    std::unique_lock<std::mutex> lk(m);
    string fname("/tmp/tmpfile");
    string artist("Some guy");
    string album("Some album");
    artist += num;
    album += num;
    fname += num;
    cv.wait(lk, []{return go;});
    for(int i=0; i<10; i++) {
        unlink(fname.c_str());
        ASSERT_TRUE(lfdl->download(artist, album, fname));
    }
    unlink(fname.c_str());
}

TEST(Downloader, threads) {
    LastFMDownloader lfdl(new FakeDownloader());
    vector<std::thread> workers;
    for(int i=0; i<10; i++) {
        workers.emplace_back(query_thread, &lfdl, i);
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
    {
        std::lock_guard<std::mutex> g(m);
        go = true;
        cv.notify_all();
    }
    for(auto &i : workers) {
        i.join();
    }
}


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
