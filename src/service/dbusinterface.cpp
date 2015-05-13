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
#include <thumbnailer.h>
#include <internal/safe_strerror.h>

#include <map>
#include <sstream>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDebug>
#include <QThreadPool>

using namespace std;
using namespace unity::thumbnailer::internal;

namespace {
const char ART_ERROR[] = "com.canonical.Thumbnailer.Error.Failed";
}

namespace unity {
namespace thumbnailer {
namespace service {

struct DBusInterfacePrivate {
    std::shared_ptr<Thumbnailer> thumbnailer = std::make_shared<Thumbnailer>();
    std::map<Handler*,std::unique_ptr<Handler>> requests;
    std::shared_ptr<QThreadPool> check_thread_pool = std::make_shared<QThreadPool>();
    std::shared_ptr<QThreadPool> create_thread_pool = std::make_shared<QThreadPool>();
};

DBusInterface::DBusInterface(QObject *parent)
    : QObject(parent), p(new DBusInterfacePrivate) {
}

DBusInterface::~DBusInterface() {
}

QDBusUnixFileDescriptor DBusInterface::GetAlbumArt(const QString &artist, const QString &album, const QSize &requestedSize) {
    qDebug() << "Look up cover art for" << artist << "/" << album << "at size" << requestedSize;
    auto request = p->thumbnailer->get_album_art(
        artist.toStdString(), album.toStdString(), requestedSize);
    queueRequest(new Handler(connection(), message(),
                             p->check_thread_pool, p->create_thread_pool,
                             std::move(request)));
    return QDBusUnixFileDescriptor();
}

QDBusUnixFileDescriptor DBusInterface::GetArtistArt(const QString &artist, const QString &album, const QSize &requestedSize) {
    qDebug() << "Look up artist art for" << artist << "/" << album << "at size" << requestedSize;
    auto request = p->thumbnailer->get_artist_art(
        artist.toStdString(), album.toStdString(), requestedSize);
    queueRequest(new Handler(connection(), message(),
                             p->check_thread_pool, p->create_thread_pool,
                             std::move(request)));
    return QDBusUnixFileDescriptor();
}

QDBusUnixFileDescriptor DBusInterface::GetThumbnail(const QString &filename, const QDBusUnixFileDescriptor &filename_fd, const QSize &requestedSize) {
    qDebug() << "Create thumbnail for" << filename << "at size" << requestedSize;

    struct stat filename_stat, fd_stat;
    if (stat(filename.toUtf8(), &filename_stat) < 0) {
        sendErrorReply(ART_ERROR, "DBusInterface::GetThumbnail(): Could not stat " +
                       filename + ": " + QString::fromStdString(safe_strerror(errno)));
        return QDBusUnixFileDescriptor();
    }
    if (fstat(filename_fd.fileDescriptor(), &fd_stat) < 0) {
        sendErrorReply(ART_ERROR, "DBusInterface::GetThumbnail(): Could not stat file descriptor: " + QString::fromStdString(safe_strerror(errno)));
        return QDBusUnixFileDescriptor();
    }
    if (filename_stat.st_dev != fd_stat.st_dev ||
        filename_stat.st_ino != fd_stat.st_ino) {
        sendErrorReply(ART_ERROR, "DBusInterface::GetThumbnail(): " + filename
                       + " refers to a different file than the file descriptor");
        return QDBusUnixFileDescriptor();
    }

    // FIXME: check that the thumbnail was produced for fd_stat
    auto request = p->thumbnailer->get_thumbnail(
        filename.toStdString(), filename_fd, requestedSize);
    queueRequest(new Handler(connection(), message(),
                             p->check_thread_pool, p->create_thread_pool,
                             std::move(request)));
    return QDBusUnixFileDescriptor();
}

void DBusInterface::queueRequest(Handler *handler) {
    p->requests.emplace(handler, std::unique_ptr<Handler>(handler));
    Q_EMIT endInactivity();
    connect(handler, &Handler::finished, this, &DBusInterface::requestFinished);
    setDelayedReply(true);
    handler->begin();
}

void DBusInterface::requestFinished() {
    Handler *handler = static_cast<Handler*>(sender());
    try {
        auto &h = p->requests.at(handler);
        h.release();
        p->requests.erase(handler);
    } catch (const std::out_of_range &e) {
        qWarning() << "finished() called on unknown handler" << handler;
    }
    if (p->requests.empty()) {
        Q_EMIT startInactivity();
    }
    // Queue deletion of handler when we re-enter the event loop.
    handler->deleteLater();
}

}
}
}
