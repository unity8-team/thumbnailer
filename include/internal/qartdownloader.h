/*
 * Copyright (C) 2015 Canonical Ltd
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
 * Authored by: Xavi Garcia <xavi.garcia.mena@canonical.com>
 */

#pragma once

#include <QObject>

#include <QNetworkAccessManager>
#include <QNetworkReply>

namespace unity
{

namespace thumbnailer
{

namespace internal
{

/**
\brief Base class to download remote files.

This class uses internally QNetworkAccessManager to retrieve remote urls.
*/
class QArtDownloader : public QObject
{
    Q_OBJECT
public:
    QArtDownloader(QObject* parent = nullptr);
    virtual ~QArtDownloader() = default;

    QArtDownloader(QArtDownloader const&) = delete;
    QArtDownloader& operator=(QArtDownloader const&) = delete;

    /**
    \brief Downloads the image for the given artist and album.
     After this call expect file_downloaded,  download_error, download_source_not_found
     or bad_url_error signals to be emitted

     \note the url is also returned in the file_downloaded and download_error signals
     so you can identify the url being downloaded or giving an error in case
     you run from multiple threads or download multiple files.

     \return the url being downloaded or an empty QString if the constructed is not
             valid
    */
    virtual QString download(QString const& artist, QString const& album) = 0;

    /**
    \brief Downloads the image of the artist for the given artist and album.
     After this call expect file_downloaded,  download_error, download_source_not_found
     or bad_url_error signals to be emitted

     \note the url is also returned in the file_downloaded and download_error signals
     so you can identify the url being downloaded or giving an error in case
     you run from multiple threads or download multiple files.

     \return the url being downloaded or an empty QString if the constructed is not
             valid
    */
    virtual QString download_artist(QString const& artist, QString const& album) = 0;

protected:
    /**
     \brief Starts the download of the given url
     \return a pointer to the QNetworkReply returned by QNetworkAccessManager
      so the user can connect to its signals or get any information, or a nullptr
      if the given url is not valid
    */
    QNetworkReply* start_download(QUrl const& url);

protected Q_SLOTS:
    void reply_finished(QNetworkReply* reply);

Q_SIGNALS:
    void file_downloaded(QString const& url, QByteArray const& data);
    void download_error(QString const& url, QNetworkReply::NetworkError error, QString const& error_message);
    void download_source_not_found(QString const& url, QNetworkReply::NetworkError error, QString const& error_message);
    void bad_url_error(QString const& error_message);

private:
    // Returns if the error is considered a connection or server error.
    bool is_server_or_connection_error(QNetworkReply::NetworkError error) const;
    QNetworkAccessManager network_manager_;
};

}  // namespace internal

}  // namespace thumbnailer

}  // namespace unity
