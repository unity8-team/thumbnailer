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

#include <internal/lastfmdownloader.h>
#include <internal/ubuntuserverdownloader.h>

#include <gtest/gtest.h>

#include <gio/gio.h>

#include <condition_variable>
#include <cstdio>
#include <fstream>
#include <mutex>
#include <thread>
#include <unistd.h>

using namespace std;
using namespace unity::thumbnailer::internal;

const char* testimage = "abc";

class FakeDownloader : public HttpDownloader
{
private:
    constexpr static const char* imloc = "http://dummy";
    constexpr static const char* xml = "<album><coverart><large>http://dummy</large></coverart></album>";

public:
    std::string download(const std::string& url) override
    {
        if (url.find("audioscrobbler") != std::string::npos)
        {
            return xml;
        }
        if (url == imloc)
        {
            return testimage;
        }
        throw std::runtime_error("Tried to get unknown data from FakeDownloader.");
    }
};

class FakeDownloader2 : public HttpDownloader
{
private:
    constexpr static const char* imloc = "http://dummy";

public:
    std::string download(const std::string& url) override
    {
        return url;  // use url as file content
    }
};

TEST(Downloader, api_key)
{
    setenv("GSETTINGS_BACKEND", "memory", 1);

    UbuntuServerDownloader ubdl(new FakeDownloader2());
    {
        string outfile("/tmp/temptestfile");

        unlink(outfile.c_str());
        std::string output = ubdl.download("foo", "bar");

        ASSERT_NE(output.find("key=0f450aa882a6125ebcbfb3d7f7aa25bc"), std::string::npos);
    }

    {
        string outfile("/tmp/temptestfile2");
        unlink(outfile.c_str());
        std::string output = ubdl.download_artist("foo", "bar");

        ASSERT_NE(output.find("key=0f450aa882a6125ebcbfb3d7f7aa25bc"), std::string::npos);
    }
}

TEST(Downloader, canned)
{
    LastFMDownloader lfdl(new FakeDownloader());
    string artist("Some guy");
    string album("Some album");
    string outfile("/tmp/temptestfile");
    unlink(outfile.c_str());
    std::string content = lfdl.download(artist, album);
    ASSERT_TRUE(!content.empty());

    std::ofstream out_stream(outfile);
    out_stream << content;
    out_stream.close();

    std::ifstream in_stream(outfile);

    string content_test(static_cast<stringstream const&>(stringstream() << in_stream.rdbuf()).str());
    in_stream.close();
    ASSERT_EQ(content, content_test);
}

static std::mutex m;
static std::condition_variable cv;
static bool go = false;

static void query_thread(LastFMDownloader* lfdl, int num)
{
    string fname("/tmp/tmpfile");
    string artist("Some guy");
    string album("Some album");
    artist += num;
    album += num;
    fname += num;
    {
        std::unique_lock<std::mutex> lk(m);
        cv.wait(lk,
                []
                {
            return go;
        });
    }
    for (int i = 0; i < 10; i++)
    {
        unlink(fname.c_str());
        ASSERT_TRUE(!lfdl->download(artist, album).empty());
    }
    unlink(fname.c_str());
}

TEST(Downloader, threads)
{
    LastFMDownloader lfdl(new FakeDownloader());
    vector<std::thread> workers;
    for (int i = 0; i < 10; i++)
    {
        workers.emplace_back(query_thread, &lfdl, i);
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
    {
        std::lock_guard<std::mutex> g(m);
        go = true;
    }
    cv.notify_all();
    for (auto& i : workers)
    {
        i.join();
    }
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
