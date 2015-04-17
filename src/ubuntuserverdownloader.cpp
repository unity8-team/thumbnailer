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

#include <gio/gio.h>

#include <internal/ubuntuserverdownloader.h>

#include <memory>
#include <iostream>

using namespace std;
using namespace unity::thumbnailer::internal;


// const strings
namespace
{
    static constexpr const char* THUMBNAILER_SCHEMA = "com.canonical.Unity.Thumbnailer";
    static constexpr const char* THUMBNAILER_API_KEY = "dash-ubuntu-com-key";
    static constexpr const char* UBUNTU_SERVER_BASE_URL = "https://dash.ubuntu.com";
    static constexpr const char* REQUESTED_ALBUM_IMAGE_SIZE = "350";
    static constexpr const char* REQUESTED_ARTIST_IMAGE_SIZE = "300";
    static constexpr const char* ALBUM_ART_BASE_URL = "musicproxy/v1/album-art";
    static constexpr const char* ARTIST_ART_BASE_URL = "musicproxy/v1/artist-art";
}

// helper methods to retrieve image urls
QString get_parameter(QString const& parameter, QString const& value)
{
    return parameter + QString("=") + value;
}

QString get_art_url(
    QString const& base_url, QString const& size, QString const& artist, QString const& album, QString const& api_key)
{
    QString prefix_api_root = UBUNTU_SERVER_BASE_URL;
    char const* apiroot_c = getenv("THUMBNAILER_UBUNTU_APIROOT");
    if (apiroot_c)
    {
        prefix_api_root = apiroot_c;
    }

    return prefix_api_root + "/" + base_url + "?" + get_parameter("artist", artist) + "&" +
           get_parameter("album", album) + "&" + get_parameter("size", size) + "&" + get_parameter("key", api_key);
}

QString get_album_art_url(QString const& artist, QString const& album, QString const& api_key)
{
    return get_art_url(ALBUM_ART_BASE_URL, REQUESTED_ALBUM_IMAGE_SIZE, artist, album, api_key);
}

QString get_artist_art_url(QString const& artist, QString const& album, QString const& api_key)
{
    return get_art_url(ARTIST_ART_BASE_URL, REQUESTED_ARTIST_IMAGE_SIZE, artist, album, api_key);
}

UbuntuServerDownloader::UbuntuServerDownloader(QObject* parent)
    : ArtDownloader(parent)
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
                api_key = QString(akey);
                status = true;
                g_free(akey);
            }
            g_object_unref(settings);
        }
        if (!status)
        {
            // TODO throw exception or emit error
            qCritical() << "Failed to get API key";
        }
    }
    else
    {
        // TODO throw exception or emit error
        qCritical() << "The schema " << THUMBNAILER_SCHEMA << " is missing";
    }
}

QString UbuntuServerDownloader::download_album(QString const& artist, QString const& album)
{
    return download(QUrl(get_album_art_url(artist, album, api_key)));
}

QString UbuntuServerDownloader::download_artist(QString const& artist, QString const& album)
{
    return download(QUrl(get_artist_art_url(artist, album, api_key)));
}
