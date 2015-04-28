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
 *              Xavi Garcia <xavi.garcia.mena@canonical.com>
 */

#include <internal/artreply.h>
#include <internal/lastfmdownloader.h>

#include <QNetworkReply>
#include <QXmlStreamReader>

#include <cassert>

namespace
{
constexpr char const NOTFOUND_IMAGE[] = "http://cdn.last.fm/flatness/catalogue/noimage/2/default_album_medium.png";
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

class LastFMArtReply : public ArtReply
{
    Q_OBJECT
public:
    Q_DISABLE_COPY(LastFMArtReply)

    LastFMArtReply(std::shared_ptr<QNetworkAccessManager> network_manager,
                   QString const& url,
                   QObject* parent = nullptr)
        : ArtReply(parent)
        , is_running_(false)
        , error_(false)
        , is_not_found_error_(false)
        , url_string_(url)
        , network_manager_(network_manager)
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

    void finish_with_error(bool is_not_found_error, QString const& error_string)
    {
        this->is_running_ = false;
        this->error_ = true;
        this->is_not_found_error_ = is_not_found_error;
        this->error_string_ = error_string;
        Q_EMIT finished();
    }

public Q_SLOTS:
    void download_xml_finished()
    {
        QNetworkReply* reply = static_cast<QNetworkReply*>(sender());
        assert(reply);

        // we got an xml.
        if (reply->error())
        {
            finish_with_error(is_not_found_error(reply->error()),
                              "LastFMArtReply::download_xml_finished() " + reply->errorString());
        }
        else
        {
            // continue parsing the xml
            auto xml_url = parse_xml(reply->readAll());
            if (!xml_url.isEmpty())
            {
                if (xml_url == NOTFOUND_IMAGE)
                {
                    finish_with_error(true, QString("LastFMArtReply::download_xml_finished() Image for %1 was not found").arg(xml_url));
                }
                else
                {
                    QUrl url(xml_url);
                    if (url.isValid())
                    {
                        QNetworkReply* image_reply = network_manager_->get(QNetworkRequest(url));
                        connect(image_reply, &QNetworkReply::finished, this, &LastFMArtReply::download_image_finished);
                    }
                    else
                    {
                        finish_with_error(
                            false, "LastFMArtReply::download_xml_finished() Bad url obtained from lastfm: " + xml_url);
                    }
                }
            }
        }
        reply->deleteLater();
    }

    void download_image_finished()
    {
        QNetworkReply* reply = static_cast<QNetworkReply*>(sender());
        assert(reply);

        this->is_running_ = false;
        this->is_not_found_error_ = is_not_found_error(reply->error());
        if (!reply->error())
        {
            this->data_ = reply->readAll();
            this->error_ = false;
        }
        else
        {
            this->error_string_ = reply->errorString();
            this->error_ = true;
        }
        Q_EMIT finished();
        reply->deleteLater();
    }

protected:
    QString parse_xml(QByteArray const& data)
    {
        const QString COVER_ART_ELEMENT = "coverart";
        const QString LARGE_IMAGE_ELEMENT = "large";

        QXmlStreamReader xml(data);
        bool parsing_coverart_node = false;
        bool found = false;
        QString image_url;
        while (!xml.atEnd() && !xml.hasError() && !found)
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
                            image_url = xml.text().toString();
                            if (!image_url.isEmpty())
                            {
                                found = true;
                            }
                        }
                    }
                }
            }
        }

        if (found)
        {
            return image_url;
        }

        if (xml.hasError())
        {
            finish_with_error(false, "LastFMArtReply::parse_xml() XML ERROR: " + xml.errorString());
        }
        else
        {
            finish_with_error(false, "LastFMArtReply::parse_xml() Image url not found");
        }
        return "";
    }

    bool is_running_;
    QString error_string_;
    bool error_;
    bool is_not_found_error_;
    QByteArray data_;
    QString url_string_;
    std::shared_ptr<QNetworkAccessManager> network_manager_;
};

LastFMDownloader::LastFMDownloader(QObject* parent)
    : ArtDownloader(parent)
    , network_manager_(new QNetworkAccessManager(this))
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
    std::shared_ptr<LastFMArtReply> art_reply(new LastFMArtReply(network_manager_, url.toString(), this));
    QNetworkReply* reply = network_manager_->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, art_reply.get(), &LastFMArtReply::download_xml_finished);

    return art_reply;
}

std::shared_ptr<ArtReply> LastFMDownloader::download_artist(QString const& /*artist*/, QString const& /*album*/)
{
    std::shared_ptr<ArtReply> ret;
    // not implemented
    return ret;
}

}  // namespace internal

}  // namespace thumbnailer

}  // namespace unity

#include "lastfmdownloader.moc"
