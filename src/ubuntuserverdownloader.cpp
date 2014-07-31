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
 * Authored by: Pawel Stolowski <pawel.stolowski@canonical.com>
 */

#include <internal/ubuntuserverdownloader.h>
#include <internal/soupdownloader.h>
#include <memory>

using namespace std;

UbuntuServerDownloader::UbuntuServerDownloader() : dl(new SoupDownloader()){
}

UbuntuServerDownloader::UbuntuServerDownloader(HttpDownloader *o) : dl(o) {
}

bool UbuntuServerDownloader::download(const string &artist, const string &album, const string &fname) {
    const char *urlTemplate = "https://dash.ubuntu.com/musicproxy/v1/album-art?artist=%s&album=%s&size=512";
    const int bufsize = 1024;
    char buf[bufsize];
    snprintf(buf, bufsize, urlTemplate, artist.c_str(), album.c_str());

    const string image(dl->download(buf));
    if (!image.empty())
    {
        FILE *f = fopen(fname.c_str(), "w");
        fwrite(image.c_str(), 1, image.length(), f);
        fclose(f);
        return true;
    }
    return false;
}

bool UbuntuServerDownloader::download_artist(const std::string &artist, const std::string &fname)
{
    return false;
}

