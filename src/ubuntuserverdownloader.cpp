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

#define UBUNTU_SERVER_BASE_URL "https://dash.ubuntu.com/"
#define REQUESTED_IMAGE_SIZE "200"
#define UBUNTU_SERVER_ALBUM_ART_URL UBUNTU_SERVER_BASE_URL "musicproxy/v1/album-art?artist=%s&album=%s&size=" REQUESTED_IMAGE_SIZE
#define UBUNTU_SERVER_ARTIST_ART_URL UBUNTU_SERVER_BASE_URL "musicproxy/v1/artist-art?artist=%s&album=%s&size=" REQUESTED_IMAGE_SIZE

using namespace std;

UbuntuServerDownloader::UbuntuServerDownloader() : dl(new SoupDownloader()){
}

UbuntuServerDownloader::UbuntuServerDownloader(HttpDownloader *o) : dl(o) {
}

bool UbuntuServerDownloader::download(const string &artist, const string &album, const string &fname) {
    const int bufsize = 1024;
    char buf[bufsize];
    snprintf(buf, bufsize, UBUNTU_SERVER_ALBUM_ART_URL, artist.c_str(), album.c_str());

    return download(buf, fname);
}

bool UbuntuServerDownloader::download_artist(const std::string &artist, const string &album, const std::string &fname)
{
    const int bufsize = 1024;
    char buf[bufsize];
    snprintf(buf, bufsize, UBUNTU_SERVER_ARTIST_ART_URL, artist.c_str(), album.c_str());

    return download(buf, fname);
}

bool UbuntuServerDownloader::download(const std::string &url, const std::string &fname)
{
    const string image(dl->download(url));
    if (!image.empty())
    {
        FILE *f = fopen(fname.c_str(), "w");
        fwrite(image.c_str(), 1, image.length(), f);
        fclose(f);
        return true;
    }
    return false;
}
