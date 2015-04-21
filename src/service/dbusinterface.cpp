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
#include "thumbnailhandler.h"
#include "albumarthandler.h"
#include "artistarthandler.h"
#include <thumbnailer.h>

#include <map>

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDebug>

using namespace std;

namespace {

const char ART_ERROR[] = "com.canonical.Thubnailer.Error.Failed";

int desiredSizeFromString(const QString &size)
{
    if (size == "small") {
        return 128;
    } else if (size == "large") {
        return 256;
    } else if (size == "xlarge") {
        return 512;
    } else if (size == "original") {
        return 0;
    }
    std::string error("Unknown size: ");
    error += size.toStdString();
    throw std::logic_error(error);
}

}

namespace unity {
namespace thumbnailer {
namespace service {

struct DBusInterfacePrivate {
    std::shared_ptr<Thumbnailer> thumbnailer = std::make_shared<Thumbnailer>();
    std::map<Handler*,std::unique_ptr<Handler>> requests;
};

DBusInterface::DBusInterface(QObject *parent)
    : QObject(parent), p(new DBusInterfacePrivate) {
}

DBusInterface::~DBusInterface() {
}

QDBusUnixFileDescriptor DBusInterface::GetAlbumArt(const QString &artist, const QString &album, const QString &desiredSize) {
    qDebug() << "Look up cover art for" << artist << "/" << album << "at size" << desiredSize;

    int size;
    try {
        size = desiredSizeFromString(desiredSize);
    } catch (const std::logic_error& e) {
        sendErrorReply(ART_ERROR, e.what());
        return QDBusUnixFileDescriptor();
    }

    queueRequest(new AlbumArtHandler(connection(), message(), p->thumbnailer,
                                     artist, album, size));
    return QDBusUnixFileDescriptor();
}

QDBusUnixFileDescriptor DBusInterface::GetArtistArt(const QString &artist, const QString &album, const QString &desiredSize) {
    qDebug() << "Look up artist art for" << artist << "/" << album << "at size" << desiredSize;

    int size;
    try {
        size = desiredSizeFromString(desiredSize);
    } catch (const std::logic_error& e) {
        sendErrorReply(ART_ERROR, e.what());
        return QDBusUnixFileDescriptor();
    }

    queueRequest(new ArtistArtHandler(connection(), message(), p->thumbnailer,
                                      artist, album, size));
    return QDBusUnixFileDescriptor();
}

QDBusUnixFileDescriptor DBusInterface::GetThumbnail(const QString &filename, const QDBusUnixFileDescriptor &filename_fd, const QString &desiredSize) {
    qDebug() << "Create thumbnail for" << filename << "at size" << desiredSize;

    int size;
    try {
        size = desiredSizeFromString(desiredSize);
    } catch (const std::logic_error& e) {
        sendErrorReply(ART_ERROR, e.what());
        return QDBusUnixFileDescriptor();
    }

    queueRequest(new ThumbnailHandler(connection(), message(), p->thumbnailer,
                                      filename, filename_fd, size));
    return QDBusUnixFileDescriptor();
}

void DBusInterface::queueRequest(Handler *handler) {
    p->requests.emplace(handler, std::unique_ptr<Handler>(handler));
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
    // Queue deletion of handler when we re-enter the event loop.
    handler->deleteLater();
}

}
}
}
