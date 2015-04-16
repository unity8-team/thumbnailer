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
class QUrlDownloader : public QObject
{
    Q_OBJECT
public:
    QUrlDownloader(QObject* parent = nullptr);
    virtual ~QUrlDownloader() = default;

    QUrlDownloader(QUrlDownloader const&) = delete;
    QUrlDownloader& operator=(QUrlDownloader const&) = delete;

    /**
     \brief Starts the download of the given url
     \return the url being downloaded, or an empty QString if the given url is not valid
    */
    QString download(QUrl const& url, QString const &download_id = "");

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

    QMap<QString, QString> download_ids_;
};

}  // namespace internal

}  // namespace thumbnailer

}  // namespace unity
