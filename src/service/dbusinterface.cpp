/*
 * Copyright (C) 2014 Canonical, Ltd.
 *
 * Authors:
 *    James Henstridge <james.henstridge@canonical.com>
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of version 3 of the GNU General Public License as published
 * by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "dbusinterface.h"
#include "handler.h"

#include <internal/safe_strerror.h>
#include <internal/thumbnailer.h>

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDebug>
#include <QThreadPool>

#include <algorithm>
#include <map>
#include <vector>
#include <sstream>

using namespace std;
using namespace unity::thumbnailer::internal;

namespace
{
const char ART_ERROR[] = "com.canonical.Thumbnailer.Error.Failed";
}

namespace unity
{
namespace thumbnailer
{
namespace service
{

struct DBusInterfacePrivate
{
    std::shared_ptr<Thumbnailer> thumbnailer = std::make_shared<Thumbnailer>();
    std::map<Handler*, std::unique_ptr<Handler>> requests;
    std::map<string, std::vector<Handler*>> request_keys;
    std::shared_ptr<QThreadPool> check_thread_pool = std::make_shared<QThreadPool>();
    std::shared_ptr<QThreadPool> create_thread_pool = std::make_shared<QThreadPool>();
};

DBusInterface::DBusInterface(QObject* parent)
    : QObject(parent)
    , p(new DBusInterfacePrivate)
{
}

DBusInterface::~DBusInterface()
{
}

QDBusUnixFileDescriptor DBusInterface::GetAlbumArt(const QString& artist,
                                                   const QString& album,
                                                   const QSize& requestedSize)
{
    qDebug() << "Look up cover art for" << artist << "/" << album << "at size" << requestedSize;
    auto request = p->thumbnailer->get_album_art(artist.toStdString(), album.toStdString(), requestedSize);
    queueRequest(new Handler(connection(), message(), p->check_thread_pool, p->create_thread_pool, std::move(request)));
    return QDBusUnixFileDescriptor();
}

QDBusUnixFileDescriptor DBusInterface::GetArtistArt(const QString& artist,
                                                    const QString& album,
                                                    const QSize& requestedSize)
{
    qDebug() << "Look up artist art for" << artist << "/" << album << "at size" << requestedSize;
    auto request = p->thumbnailer->get_artist_art(artist.toStdString(), album.toStdString(), requestedSize);
    queueRequest(new Handler(connection(), message(), p->check_thread_pool, p->create_thread_pool, std::move(request)));
    return QDBusUnixFileDescriptor();
}

QDBusUnixFileDescriptor DBusInterface::GetThumbnail(const QString& filename,
                                                    const QDBusUnixFileDescriptor& filename_fd,
                                                    const QSize& requestedSize)
{
    qDebug() << "Create thumbnail for" << filename << "at size" << requestedSize;

    std::unique_ptr<ThumbnailRequest> request;
    try
    {
        request = p->thumbnailer->get_thumbnail(filename.toStdString(), filename_fd.fileDescriptor(), requestedSize);
    }
    catch (const exception& e)
    {
        sendErrorReply(ART_ERROR, e.what());
        return QDBusUnixFileDescriptor();
    }
    queueRequest(new Handler(connection(), message(), p->check_thread_pool, p->create_thread_pool, std::move(request)));
    return QDBusUnixFileDescriptor();
}

void DBusInterface::queueRequest(Handler* handler)
{
    p->requests.emplace(handler, std::unique_ptr<Handler>(handler));
    Q_EMIT endInactivity();
    connect(handler, &Handler::finished, this, &DBusInterface::requestFinished);
    setDelayedReply(true);

    std::vector<Handler*> &requests_for_key = p->request_keys[handler->key()];
    if (requests_for_key.size() == 0)
    {
        /* There are no other concurrent requests for this item, so
         * begin immediately. */
        handler->begin();
    }
    else
    {
        /* There are other requests for this item, so chain this
         * request to wait for them to complete first.  This way we
         * can take advantage of any cached downloads or failures. */
        connect(requests_for_key.back(), &Handler::finished,
                handler, &Handler::begin);
    }
    requests_for_key.push_back(handler);
}

void DBusInterface::requestFinished()
{
    Handler* handler = static_cast<Handler*>(sender());
    try
    {
        auto& h = p->requests.at(handler);
        h.release();
        p->requests.erase(handler);
    }
    catch (const std::out_of_range& e)
    {
        qWarning() << "finished() called on unknown handler" << handler;
    }

    // Remove ourselves from the chain of requests
    std::vector<Handler*> &requests = p->request_keys[handler->key()];
    requests.erase(std::remove(requests.begin(), requests.end(), handler), requests.end());
    if (requests.size() == 0)
    {
        p->request_keys.erase(handler->key());
    }

    if (p->requests.empty())
    {
        Q_EMIT startInactivity();
    }
    // Queue deletion of handler when we re-enter the event loop.
    handler->deleteLater();
}
}
}
}
