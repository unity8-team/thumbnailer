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

#include <internal/lastfmdownloader.h>

#include <QXmlStreamReader>

#include <memory>

namespace
{
static constexpr char const* NOTFOUND_IMAGE =
    "http://cdn.last.fm/flatness/catalogue/noimage/2/default_album_medium.png";
static constexpr char const* LASTFM_APIROOT = "http://ws.audioscrobbler.com";
static constexpr char const* LASTFM_TEMPLATE_SUFFIX = "/1.0/album/%1/%2/info.xml";
static constexpr int MAX_XML_DOWNLOAD_RETRIES = 3;
}

using namespace std;
using namespace unity::thumbnailer::internal;

LastFMDownloader::LastFMDownloader(QObject* parent)
    : ArtDownloader(parent)
    , xml_downloader_(new UrlDownloader(this))
{
    connect(xml_downloader_.data(), &UrlDownloader::file_downloaded, this, &LastFMDownloader::xml_downloaded);
    // for the case of network errors we will retry
    connect(xml_downloader_.data(), &UrlDownloader::download_error, this, &LastFMDownloader::xml_download_error);

    // connect any error with the main base class
    connect(xml_downloader_.data(),
            SIGNAL(download_source_not_found(QString const&, QNetworkReply::NetworkError, QString const&)),
            this,
            SIGNAL(download_source_not_found(QString const&, QNetworkReply::NetworkError, QString const&)));
    connect(xml_downloader_.data(), SIGNAL(bad_url_error(QString const&)), this, SIGNAL(bad_url_error(QString const&)));
}

QString LastFMDownloader::parse_xml(QString const& source_url, QByteArray const& data)
{
    const QString COVER_ART_ELEMENT = "coverart";
    const QString LARGE_IMAGE_ELEMENT = "large";

    QXmlStreamReader xml(data);
    if (xml.hasError())
    {
        Q_EMIT xml_parsing_error(source_url, "LastFMDownloader::parse_xml() XML ERROR: " + xml.errorString());
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
        Q_EMIT xml_parsing_error(source_url, "LastFMDownloader::parse_xml() XML ERROR: " + xml.errorString());
        return "";
    }

    // if we are here it means the image was not found
    Q_EMIT xml_parsing_error(source_url, "LastFMDownloader::parse_xml() Image url not found");
    return "";
}

QString LastFMDownloader::download_album(QString const& artist, QString const& album)
{
    QString prefix_api_root = LASTFM_APIROOT;
    char const* apiroot_c = getenv("THUMBNAILER_LASTFM_APIROOT");
    if (apiroot_c)
    {
        prefix_api_root = apiroot_c;
    }
    QString xml_url = prefix_api_root + QString(LASTFM_TEMPLATE_SUFFIX).arg(artist).arg(album);

    retries_map_[xml_url] = MAX_XML_DOWNLOAD_RETRIES;
    // we download the xml in the dedicated UrlDownloader so the process is
    // transparent to the user
    xml_downloader_->download(xml_url);

    return xml_url;
}

QString LastFMDownloader::download_artist(QString const& /*artist*/, QString const& /*album*/)
{
    // not implemented
    return "";
}

// PRIVATE SLOTS FOR XML DOWNLOADING

void LastFMDownloader::xml_downloaded(QString const& url, QByteArray const& data)
{
    // we successfully got an xml.
    // now parse it and ask for the final image download

    // erase the entry from the retries_map
    QMap<QString, int>::iterator iter_retries = retries_map_.find(url);
    if (iter_retries != retries_map_.end())
    {
        retries_map_.erase(iter_retries);
    }

    auto xml_url = parse_xml(url, data);
    if (!xml_url.isEmpty())
    {
        if (xml_url == NOTFOUND_IMAGE)
        {
            Q_EMIT download_source_not_found(
                url, QNetworkReply::ContentNotFoundError, QString("Image for %1 was not found").arg(xml_url));
            return;
        }
        // download the image with the method in the base class
        // so any signal emitted can be caught by users

        // Note that in this case we set the identifier of the download to
        // the xml file url, as that was the one we returned initially and it's the
        // one the user will be using.
        download(QUrl(xml_url), url);
    }
}

void LastFMDownloader::xml_download_error(QString const& url,
                                          QNetworkReply::NetworkError error,
                                          QString const& error_message)
{
    // we got a network error
    // we can retry
    QMap<QString, int>::iterator iter_retries = retries_map_.find(url);
    if (iter_retries != retries_map_.end())
    {
        if ((*iter_retries)--)
        {
            // retry
            xml_downloader_->download(QUrl(url));
        }
        else
        {
            Q_EMIT download_error(url, error, error_message);
        }
    }
}
