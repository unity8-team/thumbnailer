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

class QArtDownloader : public QObject
{
    Q_OBJECT
public:
    QArtDownloader(QObject* parent = nullptr);
    virtual ~QArtDownloader() = default;

    // Returns the url of the image to download for the given artist and album
    // After this call expect file_downloaded or download_error signals to be emitted

    // Note the url is also returned in the file_downloaded and download_error signals
    // so you can identify the url being downloaded or giving an error in case
    // you run from multiple threads or download multiple files.
    virtual QString download(QString const& artist, QString const& album) = 0;

    // Returns the url of the image to download for the given artist and album
    // After this call expect file_downloaded or download_error signals to be emitted

    // Note the url is also returned in the file_downloaded and download_error signals
    // so you can identify the url being downloaded or giving an error in case
    // you run from multiple threads or download multiple files.
    virtual QString download_artist(QString const& artist, QString const& album) = 0;

    QArtDownloader(QArtDownloader const&) = delete;
    QArtDownloader& operator=(QArtDownloader const&) = delete;

protected:
    // Starts the download of the given url
    // Returns a pointer to the QNetworkReply returned by QNetworkAccessManager
    // so the user can connect to its signals or get any information
    QNetworkReply* start_download(QUrl const& url);

protected slots:
    void reply_finished(QNetworkReply* reply);

signals:
    void file_downloaded(QString const& url, QByteArray const& data);
    void download_error(QString const& url, QNetworkReply::NetworkError error, QString const& error_message);

private:
    QNetworkAccessManager network_manager_;
};

}  // namespace internal

}  // namespace thumbnailer

}  // namespace unity
