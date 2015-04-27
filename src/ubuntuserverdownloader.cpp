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

#include <internal/artreply.h>
#include <internal/ubuntuserverdownloader.h>

#include <QNetworkReply>
#include <QUrlQuery>
#include <QThread>

#include <memory>
#include <iostream>
#include <cassert>

using namespace std;

// const strings
namespace
{
    constexpr const char THUMBNAILER_SCHEMA[] = "com.canonical.Unity.Thumbnailer";
    constexpr const char THUMBNAILER_API_KEY[] = "dash-ubuntu-com-key";
    constexpr const char UBUNTU_SERVER_BASE_URL[] = "https://dash.ubuntu.com";
    constexpr const char REQUESTED_ALBUM_IMAGE_SIZE[] = "350";
    constexpr const char REQUESTED_ARTIST_IMAGE_SIZE[] = "300";
    constexpr const char ALBUM_ART_BASE_URL[] = "musicproxy/v1/album-art";
    constexpr const char ARTIST_ART_BASE_URL[] = "musicproxy/v1/artist-art";
}

namespace unity
{

namespace thumbnailer
{

namespace internal
{

class UbuntuServerArtReply : public ArtReply
{
    Q_OBJECT
public:
    Q_DISABLE_COPY(UbuntuServerArtReply)

    UbuntuServerArtReply(QObject *parent = nullptr)
        : ArtReply(parent),
          is_running_(false),
          error_(QNetworkReply::NoError)
    {
    }

    virtual ~UbuntuServerArtReply() = default;

    bool succeded() const override
    {
        return error_ == QNetworkReply::NoError;
    };

    bool is_running() const override
    {
        return is_running_;
    };

    QString error_string() const override
    {
        return error_string_;
    }

    bool not_found_error() const override
    {
        switch (error_)
        {
            // add here all the cases that you consider as source not found
            case QNetworkReply::HostNotFoundError:
            case QNetworkReply::ContentAccessDenied:
            case QNetworkReply::ContentOperationNotPermittedError:
            case QNetworkReply::ContentNotFoundError:
            case QNetworkReply::ContentGoneError:
                return true;
                break;
            default:
                return false;
                break;
        }
    }

    QByteArray const& data() const override
    {
        return data_;
    }

    QString url_string() const override
    {
        return url_string_;
    }

    void finish()
    {
        Q_EMIT finished();
    }

    bool is_running_;
    QString error_string_;
    QNetworkReply::NetworkError error_;
    QByteArray data_;
    QString url_string_;
};

// helper methods to retrieve image urls
QUrl get_art_url(
    QString const& base_url, QString const& size, QString const& artist, QString const& album, QString const& api_key)
{
    QString prefix_api_root = UBUNTU_SERVER_BASE_URL;
    char const* apiroot_c = getenv("THUMBNAILER_UBUNTU_APIROOT");
    if (apiroot_c)
    {
        prefix_api_root = apiroot_c;
    }

    QUrlQuery q;
    q.addQueryItem("artist", artist);
    q.addQueryItem("album", album);
    q.addQueryItem("size", size);
    q.addQueryItem("key", api_key);

    QUrl url(prefix_api_root + "/" + base_url);
    url.setQuery(q);
    return url;
}

QUrl get_album_art_url(QString const& artist, QString const& album, QString const& api_key)
{
    return get_art_url(ALBUM_ART_BASE_URL, REQUESTED_ALBUM_IMAGE_SIZE, artist, album, api_key);
}

QUrl get_artist_art_url(QString const& artist, QString const& album, QString const& api_key)
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
                api_key_ = QString(akey);
                status = true;
                g_free(akey);
            }
            g_object_unref(settings);
        }
        if (!status)
        {
            // TODO do something with the error
            qCritical() << "Failed to get API key";
        }
    }
    else
    {
        // TODO do something with the error
        qCritical() << "The schema " << THUMBNAILER_SCHEMA << " is missing";
    }
}

shared_ptr<ArtReply> UbuntuServerDownloader::download_album(QString const& artist, QString const& album)
{
    return download_url(get_album_art_url(artist, album, api_key_));
}

shared_ptr<ArtReply> UbuntuServerDownloader::download_artist(QString const& artist, QString const& album)
{
    return download_url(get_artist_art_url(artist, album, api_key_));
}

void UbuntuServerDownloader::download_finished(QNetworkReply* reply)
{
    std::map<QNetworkReply *, shared_ptr<UbuntuServerArtReply>>::iterator iter = replies_map_.find(reply);
    if (iter != replies_map_.end())
    {
        (*iter).second->is_running_ = false;
        (*iter).second->error_ = reply->error();
        if (!reply->error())
        {
            (*iter).second->data_ = reply->readAll();
        }
        else
        {
            (*iter).second->error_string_ = reply->errorString();
        }
        (*iter).second->finish();
        replies_map_.erase(iter);
    }
    reply->deleteLater();
}

shared_ptr<ArtReply> UbuntuServerDownloader::download_url(QUrl const& url)
{
    assert_valid_url(url);
    std::shared_ptr<UbuntuServerArtReply> art_reply(new UbuntuServerArtReply());
    QNetworkReply* reply = network_manager_.get(QNetworkRequest(url));
    connect(&network_manager_, &QNetworkAccessManager::finished, this, &UbuntuServerDownloader::download_finished);
    art_reply->is_running_ = true;
    art_reply->url_string_ = url.toString();
    replies_map_[reply] = art_reply;

    return art_reply;
}

}  // namespace internal

}  // namespace thumbnailer

}  // namespace unity

#include "ubuntuserverdownloader.moc"
