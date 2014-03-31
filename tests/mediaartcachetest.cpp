/*
 * Copyright (C) 2013-2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Jussi Pakkanen <jussi.pakkanen@canonical.com>
 */

#include<sys/types.h>
#include<string.h>
#include<stdio.h>
#include<dirent.h>

#include<string>
#include <gtest/gtest.h>

#include "../include/internal/mediaartcache.h"

using namespace std;

namespace mediascanner {

TEST(MediaArtCacheTest, BasicFunctionality) {
    MediaArtCache mac;
    mac.clear();
    string artist = "Some Guy";
    string album = "Muzak";
    const int datasize = 3;
    char data[datasize] = {'a', 'b', 'c'};
    char indata[datasize];
    FILE *f;

    ASSERT_FALSE(mac.has_art(artist, album));
    mac.add_art(artist, album, data, datasize);
    ASSERT_TRUE(mac.has_art(artist, album));
    f = fopen(mac.get_art_file(artist, album).c_str(), "r");
    ASSERT_TRUE(f);
    ASSERT_EQ(fread(indata, 1, datasize, f), datasize);
    fclose(f);
    ASSERT_EQ(memcmp(indata, data, datasize), 0);

    mac.clear();
    assert(!mac.has_art(artist, album));
}

TEST(MediaArtCacheTest, Swapped) {
    string artist1("foo");
    string album1("bar");
    string artist2(album1);
    string album2(artist1);
    const int datasize = 4;
    char data1[datasize] = {'a', 'b', 'c', 'd'};
    char data2[datasize] = {'d', 'c', 'b', 'e'};
    char indata[datasize];
    MediaArtCache mac;
    FILE *f;
    mac.clear();

    ASSERT_FALSE(mac.has_art(artist1, album1));
    ASSERT_FALSE(mac.has_art(artist2, album2));

    mac.add_art(artist1, album1, data1, datasize);
    ASSERT_TRUE(mac.has_art(artist1, album1));
    ASSERT_FALSE(mac.has_art(artist2, album2));

    mac.clear();

    mac.add_art(artist2, album2, data2, datasize);
    ASSERT_FALSE(mac.has_art(artist1, album1));
    ASSERT_TRUE(mac.has_art(artist2, album2));

    mac.add_art(artist1, album1, data1, datasize);

    f = fopen(mac.get_art_file(artist1, album1).c_str(), "r");
    ASSERT_TRUE(f);
    ASSERT_EQ(fread(indata, 1, datasize, f), datasize);
    fclose(f);
    ASSERT_EQ(memcmp(indata, data1, datasize), 0);

    f = fopen(mac.get_art_file(artist2, album2).c_str(), "r");
    ASSERT_TRUE(f);
    ASSERT_EQ(fread(indata, 1, datasize, f), datasize);
    fclose(f);
    ASSERT_EQ(memcmp(indata, data2, datasize), 0);
}

static int count_files(const string &dir) {
    DIR *d = opendir(dir.c_str());
    int count = 0;
    if(!d) {
        string s = "Something went wrong.";
        throw s;
    }
    struct dirent *entry, *de;
    entry = (dirent*)malloc(sizeof(dirent) + NAME_MAX);
    while(readdir_r(d, entry, &de) == 0 && de) {
        string basename = entry->d_name;
        if (basename == "." || basename == "..")
            continue;
        count++;
    }
    closedir(d);
    free(entry);
    return count;
}

TEST(MediaArtCacheTest, Prune) {
    MediaArtCache mac;
    mac.clear();
    const int max_files = MediaArtCache::MAX_SIZE;
    string cache_dir = mac.get_cache_dir();
    char arr[] = {'a', 'b', 'c'};
    ASSERT_EQ(count_files(cache_dir), 0);
    for(int i=0; i<max_files+5; i++) {
        string tmpname = cache_dir + "/" + to_string(i) + ".jpg";
        FILE *f = fopen(tmpname.c_str(), "w");
        if (!f) {
            string error("Could not create file ");
            error += tmpname;
            throw error;
        }
        fwrite(arr, 1, 3, f);
        fflush(f);
        fclose(f);
    }
    ASSERT_GT(count_files(cache_dir), max_files);
    mac.prune();
    int numfiles = count_files(cache_dir);
    ASSERT_GT(numfiles, 0);
    ASSERT_LE(numfiles, max_files);
    mac.prune();
    ASSERT_EQ(numfiles, count_files(cache_dir));
}

}

int main(int argc, char *argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
