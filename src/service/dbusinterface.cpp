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

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDebug>
#include <unity/util/ResourcePtr.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace std;
using namespace unity::thumbnailer::service;

static const char ART_ERROR[] = "com.canonical.MediaScanner2.Error.Failed";

struct DBusInterfacePrivate {
    std::shared_ptr<Thumbnailer> thumbnailer = std::make_shared<Thumbnailer>();
};

namespace {

ThumbnailSize desiredSizeFromString(const QString &size)
{
    if (size == "small") {
        return TN_SIZE_SMALL;
    } else if (size == "large") {
        return TN_SIZE_LARGE;
    } else if (size == "xlarge") {
        return TN_SIZE_XLARGE;
    } else if (size == "original") {
        return TN_SIZE_ORIGINAL;
    }
    std::string error("Unknown size: ");
    error += size.toStdString();
    throw std::logic_error(error);
}

}

DBusInterface::DBusInterface(QObject *parent)
    : QObject(parent), p(new DBusInterfacePrivate) {
}

DBusInterface::~DBusInterface() {
}

QDBusUnixFileDescriptor DBusInterface::GetAlbumArt(const QString &artist, const QString &album, const QString &desiredSize) {
    qDebug() << "Look up cover art for" << artist << "/" << album << "at size" << desiredSize;

    ThumbnailSize size;
    try {
        size = desiredSizeFromString(desiredSize);
    } catch (const std::logic_error& e) {
        sendErrorReply(ART_ERROR, e.what());
        return QDBusUnixFileDescriptor();
    }

    auto handler = new AlbumArtHandler(
        connection(), message(), p->thumbnailer, artist, album, size);
    setDelayedReply(true);
    handler->begin();
    return QDBusUnixFileDescriptor();
}

QDBusUnixFileDescriptor DBusInterface::GetArtistArt(const QString &artist, const QString &album, const QString &desiredSize) {
    qDebug() << "Look up artist art for" << artist << "/" << album << "at size" << desiredSize;

    ThumbnailSize size;
    try {
        size = desiredSizeFromString(desiredSize);
    } catch (const std::logic_error& e) {
        sendErrorReply(ART_ERROR, e.what());
        return QDBusUnixFileDescriptor();
    }

    auto handler = new ArtistArtHandler(
        connection(), message(), p->thumbnailer, artist, album, size);
    setDelayedReply(true);
    handler->begin();
    return QDBusUnixFileDescriptor();
}

QDBusUnixFileDescriptor DBusInterface::GetThumbnail(const QString &filename, const QDBusUnixFileDescriptor &filename_fd, const QString &desiredSize) {
    qDebug() << "Look thumbnail for" << filename << "at size" << desiredSize;

    ThumbnailSize size;
    try {
        size = desiredSizeFromString(desiredSize);
    } catch (const std::logic_error& e) {
        sendErrorReply(ART_ERROR, e.what());
        return QDBusUnixFileDescriptor();
    }

    auto handler = new ThumbnailHandler(
        connection(), message(), p->thumbnailer, filename, filename_fd, size);
    setDelayedReply(true);
    handler->begin();
    return QDBusUnixFileDescriptor();
}
