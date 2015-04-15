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

#include <internal/qurldownloader.h>

#include <QNetworkRequest>

using namespace unity::thumbnailer::internal;

QUrlDownloader::QUrlDownloader(QObject* parent)
    : QObject(parent)
{
}

QString QUrlDownloader::download(QUrl const& url)
{
    // first of all check that the URL is valid
    if (!url.isValid())
    {
        Q_EMIT bad_url_error(url.errorString());
        return "";
    }
    // start downloading a file using the QNetworkAccessManager
    QNetworkReply* reply = network_manager_.get(QNetworkRequest(url));
    connect(&network_manager_, &QNetworkAccessManager::finished, this, &QUrlDownloader::reply_finished);
    return reply->url().toString();
}

void QUrlDownloader::reply_finished(QNetworkReply* reply)
{
    if (!reply->error())
    {
        QByteArray data = reply->readAll();
        Q_EMIT file_downloaded(reply->url().toString(), data);
    }
    else
    {
        // we return the url in the original request as the url in
        // the reply may change
        if (is_server_or_connection_error(reply->error()))
        {
            Q_EMIT download_error(reply->request().url().toString(), reply->error(), reply->errorString());
        }
        else
        {
            Q_EMIT download_source_not_found(reply->request().url().toString(), reply->error(), reply->errorString());
        }
    }
    reply->deleteLater();
}

bool QUrlDownloader::is_server_or_connection_error(QNetworkReply::NetworkError error) const
{
    switch (error)
    {
    // add here all the cases that you consider as source not found
    case QNetworkReply::HostNotFoundError:
    case QNetworkReply::ContentAccessDenied:
    case QNetworkReply::ContentOperationNotPermittedError:
    case QNetworkReply::ContentNotFoundError:
    case QNetworkReply::ContentGoneError:
        return false;
        break;
    default:
        return true;
        break;
    }
}
