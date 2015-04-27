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

#include <internal/artreply.h>
#include <internal/lastfmdownloader.h>

#include <QNetworkReply>
#include <QXmlStreamReader>

#include <cassert>

namespace
{
    constexpr char const NOTFOUND_IMAGE[] =
        "http://cdn.last.fm/flatness/catalogue/noimage/2/default_album_medium.png";
    constexpr char const LASTFM_APIROOT[] = "http://ws.audioscrobbler.com";
    constexpr char const LASTFM_TEMPLATE_SUFFIX[] = "/1.0/album/%1/%2/info.xml";
    constexpr int MAX_XML_DOWNLOAD_RETRIES = 3;
}

using namespace std;

namespace unity
{

namespace thumbnailer
{

namespace internal
{

class LastFMArtReply : public ArtReply
{
    Q_OBJECT
public:
    Q_DISABLE_COPY(LastFMArtReply)

    LastFMArtReply(QObject *parent = nullptr)
        : ArtReply(parent),
          is_running_(false),
          error_(false),
          is_not_found_error_(false)
    {
    }

    virtual ~LastFMArtReply() = default;

    bool succeded() const override
    {
        return !error_;
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
        return is_not_found_error_;
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
    bool error_;
    bool is_not_found_error_;
    QByteArray data_;
    QString url_string_;
};

void finish_lastfm_reply_with_error(std::shared_ptr<LastFMArtReply> const &lastfm_reply, bool is_not_found_error, QString const& error_string)
{
    lastfm_reply->is_running_ = false;
    lastfm_reply->error_ = true;
    lastfm_reply->is_not_found_error_ = is_not_found_error;
    lastfm_reply->error_string_ = error_string;
    lastfm_reply->finish();
}

QString parse_xml(std::shared_ptr<LastFMArtReply> const &lastfm_reply, QByteArray const& data)
{
    const QString COVER_ART_ELEMENT = "coverart";
    const QString LARGE_IMAGE_ELEMENT = "large";

    QXmlStreamReader xml(data);
    if (xml.hasError())
    {
        finish_lastfm_reply_with_error(lastfm_reply, false, "LastFMDownloader::parse_xml() XML ERROR: " + xml.errorString());
        return "";
    }

    bool parsing_coverart_node = false;
    while (!xml.atEnd() && !xml.hasError())
    {
        /* Read next element.*/
        QXmlStreamReader::TokenType token = xml.readNext();
        if (token == QXmlStreamReader::StartElement)
        {
            if (xml.name() == COVER_ART_ELEMENT)
            {
                parsing_coverart_node = true;
            }
            else if (xml.name() == LARGE_IMAGE_ELEMENT)
            {
                if (parsing_coverart_node)
                {
                    // this is the element we are looking for
                    token = xml.readNext();
                    if (token == QXmlStreamReader::Characters)
                    {
                        QString image_url = xml.text().toString();
                        if (!image_url.isEmpty())
                        {
                            // return the image
                            return image_url;
                        }
                    }
                }
            }
        }
        else if (token == QXmlStreamReader::EndElement)
        {
            if (xml.name() == COVER_ART_ELEMENT)
            {
                parsing_coverart_node = false;
            }
        }
    }

    if (xml.hasError())
    {
        finish_lastfm_reply_with_error(lastfm_reply, false, "LastFMDownloader::parse_xml() XML ERROR: " + xml.errorString());
    }
    else
    {
        finish_lastfm_reply_with_error(lastfm_reply, false, "LastFMDownloader::parse_xml() Image url not found");
    }
    return "";
}

bool is_not_found_error(QNetworkReply::NetworkError error)
{
    switch (error)
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

LastFMDownloader::LastFMDownloader(QObject* parent)
    : ArtDownloader(parent)
{
}

std::shared_ptr<ArtReply> LastFMDownloader::download_album(QString const& artist, QString const& album)
{
    QString prefix_api_root = LASTFM_APIROOT;
    char const* apiroot_c = getenv("THUMBNAILER_LASTFM_APIROOT");
    if (apiroot_c)
    {
        prefix_api_root = apiroot_c;
    }

    QUrl url(prefix_api_root + QString(LASTFM_TEMPLATE_SUFFIX).arg(artist).arg(album));

    assert_valid_url(url);
    std::shared_ptr<LastFMArtReply>art_reply(new LastFMArtReply());
    QNetworkReply* reply = network_manager_.get(QNetworkRequest(url));
    connect(&network_manager_, &QNetworkAccessManager::finished, this, &LastFMDownloader::download_finished);
    art_reply->is_running_ = true;
    art_reply->url_string_ = url.toString();
    replies_xml_map_[reply] = art_reply;

    return art_reply;
}

std::shared_ptr<ArtReply> LastFMDownloader::download_artist(QString const& /*artist*/, QString const& /*album*/)
{
    std::shared_ptr<ArtReply> ret;
    // not implemented
    return ret;
}

// SLOTS FOR XML DOWNLOADING

void LastFMDownloader::download_finished(QNetworkReply* reply)
{
    // check which type of download we've got
    IterReply iter = replies_xml_map_.find(reply);
    if (iter != replies_xml_map_.end())
    {
        download_xml_finished(reply, iter);
    }
    else
    {
        iter = replies_image_map_.find(reply);
        if (iter != replies_image_map_.end())
        {
            download_image_finished(reply, iter);
        }
    }
}

void LastFMDownloader::download_xml_finished(QNetworkReply *reply, IterReply const& iter)
{
    // we got an xml.
    if (reply->error())
    {
        finish_lastfm_reply_with_error((*iter), is_not_found_error(reply->error()), "LastFMDownloader::download_xml_finished() " + reply->errorString());
    }
    else
    {
        // continue parsing the xml
        auto xml_url = parse_xml((*iter), reply->readAll());
        if (!xml_url.isEmpty())
        {
            if (xml_url == NOTFOUND_IMAGE)
            {
                finish_lastfm_reply_with_error((*iter), true, QString("Image for %1 was not found").arg(xml_url));
            }
            else
            {
                QUrl url(xml_url);
                if (url.isValid())
                {
                    QNetworkReply* image_reply = network_manager_.get(QNetworkRequest(url));
                    connect(&network_manager_, &QNetworkAccessManager::finished, this, &LastFMDownloader::download_finished);
                    replies_image_map_[image_reply] = (*iter);
                }
                else
                {
                    finish_lastfm_reply_with_error((*iter), false,
                            "LastFMDownloader::download_xml_finished() Bad url obtained from lastfm: " + xml_url);
                }
            }
        }
    }
    replies_xml_map_.erase(iter);
    reply->deleteLater();
}

void LastFMDownloader::download_image_finished(QNetworkReply *reply, IterReply const& iter)
{
    (*iter)->is_running_ = false;
    if (!reply->error())
    {
        (*iter)->data_ = reply->readAll();
        (*iter)->error_ = false;
    }
    else
    {
        (*iter)->error_string_ = reply->errorString();
        (*iter)->error_ = true;
    }
    (*iter)->finish();
    replies_image_map_.erase(iter);
    reply->deleteLater();
}

}  // namespace internal

}  // namespace thumbnailer

}  // namespace unity

#include "lastfmdownloader.moc"
