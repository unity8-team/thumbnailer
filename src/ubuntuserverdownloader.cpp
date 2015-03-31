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
#include <iostream>
#include <gio/gio.h>

#define THUMBNAILER_SCHEMA "com.canonical.Unity.Thumbnailer"
#define THUMBNAILER_API_KEY "dash-ubuntu-com-key"
#define UBUNTU_SERVER_BASE_URL "https://dash.ubuntu.com/"
#define REQUESTED_ALBUM_IMAGE_SIZE "350"
#define REQUESTED_ARTIST_IMAGE_SIZE "300"
#define UBUNTU_SERVER_ALBUM_ART_URL \
    UBUNTU_SERVER_BASE_URL "musicproxy/v1/album-art?artist=%s&album=%s&size=" REQUESTED_ALBUM_IMAGE_SIZE "&key=%s"
#define UBUNTU_SERVER_ARTIST_ART_URL \
    UBUNTU_SERVER_BASE_URL "musicproxy/v1/artist-art?artist=%s&album=%s&size=" REQUESTED_ARTIST_IMAGE_SIZE "&key=%s"

using namespace std;
using namespace unity::thumbnailer::internal;

UbuntuServerDownloader::UbuntuServerDownloader()
    : dl(new SoupDownloader())
{
    set_api_key();
}

UbuntuServerDownloader::UbuntuServerDownloader(HttpDownloader* o)
    : dl(o)
{
    set_api_key();
}

void UbuntuServerDownloader::set_api_key()
{
    // the API key is not expected to change, so don't monitor it
    GSettingsSchemaSource* src = g_settings_schema_source_get_default();
    GSettingsSchema* schema = g_settings_schema_source_lookup(src, THUMBNAILER_SCHEMA, true);

    if (schema)
    {
        bool status = false;
        g_settings_schema_unref(schema);
        GSettings* settings = g_settings_new(THUMBNAILER_SCHEMA);
        if (settings)
        {
            gchar* akey = g_settings_get_string(settings, THUMBNAILER_API_KEY);
            if (akey)
            {
                api_key = std::string(akey);
                status = true;
                g_free(akey);
            }
            g_object_unref(settings);
        }
        if (!status)
        {
            std::cerr << "Failed to get API key" << std::endl;
        }
    }
    else
    {
        std::cerr << "The schema " << THUMBNAILER_SCHEMA << " is missing" << std::endl;
    }
}

std::string UbuntuServerDownloader::download(string const& artist, string const& album)
{
    int const bufsize = 1024;
    char buf[bufsize];
    snprintf(buf, bufsize, UBUNTU_SERVER_ALBUM_ART_URL, artist.c_str(), album.c_str(), api_key.c_str());

    return download(buf);
}

std::string UbuntuServerDownloader::download_artist(std::string const& artist, string const& album)
{
    int const bufsize = 1024;
    char buf[bufsize];
    snprintf(buf, bufsize, UBUNTU_SERVER_ARTIST_ART_URL, artist.c_str(), album.c_str(), api_key.c_str());

    return download(buf);
}

std::string UbuntuServerDownloader::download(std::string const& url)
{
    return dl->download(url);
}
