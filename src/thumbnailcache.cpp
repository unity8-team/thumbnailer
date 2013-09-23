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

#include<internal/thumbnailcache.h>
#include<stdexcept>
#include<glib.h>
#include<sys/stat.h>

using namespace std;

class ThumbnailCachePrivate {
public:
    string tndir;
    string smalldir;
    string largedir;

    ThumbnailCachePrivate() {
        string xdg_base = g_get_user_cache_dir();
        if (xdg_base == "") {
            string s("Could not determine cache dir.");
            throw runtime_error(s);
        }
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
        ec = mkdir(tndir.c_str(), S_IRUSR | S_IWUSR | S_IXUSR);
        if (ec < 0 && errno != EEXIST) {
            string s("Could not create small dir.");
            throw runtime_error(s);
        }
        largedir = tndir + "/large";
        ec = mkdir(tndir.c_str(), S_IRUSR | S_IWUSR | S_IXUSR);
        if (ec < 0 && errno != EEXIST) {
            string s("Could not create large dir.");
            throw runtime_error(s);
        }
    };
};

ThumbnailCache::ThumbnailCache() : p() {

}
