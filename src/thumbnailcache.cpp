/*
 * Copyright (C) 2013 Canonical Ltd.
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

#include<thumbnailer.h>
#include<internal/thumbnailcache.h>
#include<stdexcept>
#include<glib.h>
#include<sys/stat.h>
#include<cassert>
#include<cstdio>

using namespace std;

class ThumbnailCachePrivate {
public:
    string tndir;
    string smalldir;
    string largedir;

    ThumbnailCachePrivate();
    string md5(const string &str) const;
    string cache_filename(const std::string &original, ThumbnailSize desired) const;
};

ThumbnailCachePrivate::ThumbnailCachePrivate() {
    string xdg_base = g_get_user_cache_dir();
    if (xdg_base == "") {
        string s("Could not determine cache dir.");
        throw runtime_error(s);
    }
    printf("%s\n", xdg_base.c_str());
    int ec = mkdir(xdg_base.c_str(), S_IRUSR | S_IWUSR | S_IXUSR);
    if (ec < 0 && errno != EEXIST) {
        string s("Could not create base dir.");
        throw runtime_error(s);
    }
    tndir = xdg_base + "/thumbnails";
    ec = mkdir(tndir.c_str(), S_IRUSR | S_IWUSR | S_IXUSR);
    if (ec < 0 && errno != EEXIST) {
        string s("Could not create thumbnail dir.");
        throw runtime_error(s);
    }
    smalldir = tndir + "/normal";
    ec = mkdir(smalldir.c_str(), S_IRUSR | S_IWUSR | S_IXUSR);
    if (ec < 0 && errno != EEXIST) {
        string s("Could not create small dir.");
        throw runtime_error(s);
    }
    largedir = tndir + "/large";
    ec = mkdir(largedir.c_str(), S_IRUSR | S_IWUSR | S_IXUSR);
    if (ec < 0 && errno != EEXIST) {
        string s("Could not create large dir.");
        throw runtime_error(s);
    }
}

string ThumbnailCachePrivate::md5(const string &str) const {
    const unsigned char *buf = (const unsigned char *)str.c_str();
    char *normalized = g_utf8_normalize((const gchar*)buf, str.size(), G_NORMALIZE_ALL);
    string final;
    gchar *result;

    if(normalized) {
        buf = (const unsigned char*)normalized;
    }
    gssize bytes = str.length();

    result = g_compute_checksum_for_data(G_CHECKSUM_MD5, buf, bytes);
    final = result;
    g_free((gpointer)normalized);
    g_free(result);
    return final;
}

string ThumbnailCachePrivate::cache_filename(const std::string & abs_original, ThumbnailSize desired) const {
    assert(abs_original[0] == '/');
    string path = desired == TN_SIZE_SMALL ? smalldir : largedir;
    path += "/" + md5("file://" + abs_original) + ".png";
    return path;
}

ThumbnailCache::ThumbnailCache() : p(new ThumbnailCachePrivate()) {
}

ThumbnailCache::~ThumbnailCache() {
    delete p;
}
