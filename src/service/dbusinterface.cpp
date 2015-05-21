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

#include <internal/safe_strerror.h>

#include <QDebug>

#include <map>
#include <sstream>

using namespace std;
using namespace unity::thumbnailer::internal;

namespace
{
char const ART_ERROR[] = "com.canonical.Thumbnailer.Error.Failed";
}

namespace unity
{

namespace thumbnailer
{

namespace service
{

DBusInterface::DBusInterface(shared_ptr<Thumbnailer> const& thumbnailer, QObject* parent)
    : QObject(parent)
    , thumbnailer_(thumbnailer)
    , check_thread_pool_(make_shared<QThreadPool>())
    , create_thread_pool_(make_shared<QThreadPool>())
{
}

DBusInterface::~DBusInterface()
{
}

QDBusUnixFileDescriptor DBusInterface::GetAlbumArt(QString const& artist,
                                                   QString const& album,
                                                   QSize const& requestedSize)
{
    qDebug() << "Look up cover art for" << artist << "/" << album << "at size" << requestedSize;
    auto request = thumbnailer_->get_album_art(artist.toStdString(), album.toStdString(), requestedSize);
    queueRequest(new Handler(connection(), message(), check_thread_pool_, create_thread_pool_, std::move(request)));
    return QDBusUnixFileDescriptor();
}

QDBusUnixFileDescriptor DBusInterface::GetArtistArt(QString const& artist,
                                                    QString const& album,
                                                    QSize const& requestedSize)
{
    qDebug() << "Look up artist art for" << artist << "/" << album << "at size" << requestedSize;
    auto request = thumbnailer_->get_artist_art(artist.toStdString(), album.toStdString(), requestedSize);
    queueRequest(new Handler(connection(), message(), check_thread_pool_, create_thread_pool_, std::move(request)));
    return QDBusUnixFileDescriptor();
}

QDBusUnixFileDescriptor DBusInterface::GetThumbnail(QString const& filename,
                                                    QDBusUnixFileDescriptor const& filename_fd,
                                                    QSize const& requestedSize)
{
    qDebug() << "Create thumbnail for" << filename << "at size" << requestedSize;

    std::unique_ptr<ThumbnailRequest> request;
    try
    {
        request = thumbnailer_->get_thumbnail(filename.toStdString(), filename_fd.fileDescriptor(), requestedSize);
    }
    catch (exception const& e)
    {
        sendErrorReply(ART_ERROR, e.what());
        return QDBusUnixFileDescriptor();
    }
    queueRequest(new Handler(connection(), message(), check_thread_pool_, create_thread_pool_, std::move(request)));
    return QDBusUnixFileDescriptor();
}

void DBusInterface::queueRequest(Handler* handler)
{
    requests.emplace(handler, std::unique_ptr<Handler>(handler));
    Q_EMIT endInactivity();
    connect(handler, &Handler::finished, this, &DBusInterface::requestFinished);
    setDelayedReply(true);
    handler->begin();
}

void DBusInterface::requestFinished()
{
    Handler* handler = static_cast<Handler*>(sender());
    try
    {
        auto& h = requests.at(handler);
        h.release();
        requests.erase(handler);
    }
    catch (std::out_of_range const& e)
    {
        qWarning() << "finished() called on unknown handler" << handler;
    }
    if (requests.empty())
    {
        Q_EMIT startInactivity();
    }
    // Queue deletion of handler when we re-enter the event loop.
    handler->deleteLater();
}

}  // namespace service

}  // namespace thumbnailer

}  // namespace unity
