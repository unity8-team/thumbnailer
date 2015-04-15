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

#include <internal/qartdownloader.h>

#include <QNetworkRequest>

using namespace unity::thumbnailer::internal;

QArtDownloader::QArtDownloader(QObject* parent)
    : QObject(parent)
{
}

QNetworkReply* QArtDownloader::start_download(QUrl const& url)
{
    // start downloading a file using the QNetworkAccessManager
    QNetworkReply* reply = network_manager_.get(QNetworkRequest(url));
    connect(&network_manager_, &QNetworkAccessManager::finished, this, &QArtDownloader::reply_finished);
    return reply;
}

void QArtDownloader::reply_finished(QNetworkReply* reply)
{
    if (!reply->error())
    {
        QByteArray data = reply->readAll();
        Q_EMIT file_downloaded(reply->url().toString(), data);
    }
    else
    {
        Q_EMIT download_error(reply->url().toString(), reply->error(), reply->errorString());
    }
    reply->deleteLater();
}
