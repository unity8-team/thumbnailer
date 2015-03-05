/*
 * Copyright 2014 Canonical Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Jussi Pakkanen <jussi.pakkanen@canonical.com>
 *          James Henstridge <james.henstridge@canonical.com>
*/

#include "albumartgenerator.h"
#include "artgeneratorcommon.h"
#include <stdexcept>
#include <QDebug>
#include <QFile>
#include <QUrlQuery>
#include <QDBusUnixFileDescriptor>
#include <QDBusReply>

static const char DEFAULT_ALBUM_ART[] = "/usr/share/thumbnailer/icons/album_missing.png";

static const char BUS_NAME[] = "com.canonical.Thumbnailer";
static const char BUS_PATH[] = "/com/canonical/Thumbnailer";
static const char THUMBNAILER_IFACE[] = "com.canonical.Thumbnailer";
static const char GET_ALBUM_ART[] = "GetAlbumArt";

AlbumArtGenerator::AlbumArtGenerator()
    : QQuickImageProvider(QQuickImageProvider::Image, QQmlImageProviderBase::ForceAsynchronousImageLoading),
      connection(nullptr),
      iface(nullptr) {
}

AlbumArtGenerator::~AlbumArtGenerator()
{
    delete iface;
    delete connection;
}

static QImage fallbackImage(QSize *realSize) {
    QImage fallback;
    fallback.load(DEFAULT_ALBUM_ART);
    *realSize = fallback.size();
    return fallback;
}

QImage AlbumArtGenerator::requestImage(const QString &id, QSize *realSize,
        const QSize &requestedSize) {
    QUrlQuery query(id);
    if (!query.hasQueryItem("artist") || !query.hasQueryItem("album")) {
        qWarning() << "Invalid albumart uri:" << id;
        return fallbackImage(realSize);
    }

    if (!connection) {
        // Create them here and not them on the constrcutor so they belong to the proper thread
        connection = new QDBusConnection(QDBusConnection::connectToBus(QDBusConnection::SessionBus, "album_art_generator_dbus_connection"));
        iface = new QDBusInterface(BUS_NAME, BUS_PATH, THUMBNAILER_IFACE, *connection);
    }

    const QString artist = query.queryItemValue("artist", QUrl::FullyDecoded);
    const QString album = query.queryItemValue("album", QUrl::FullyDecoded);

    QString desiredSize = sizeToDesiredSizeString(requestedSize);

    // perform dbus call
    QDBusReply<QDBusUnixFileDescriptor> reply = iface->call(
        GET_ALBUM_ART, artist, album, desiredSize);
    if (!reply.isValid()) {
        qWarning() << "D-Bus error: " << reply.error().message();
        return fallbackImage(realSize);
    }

    try {
        return imageFromFd(reply.value().fileDescriptor(), realSize);
    } catch (const std::exception &e) {
        qDebug() << "Album art loader failed: " << e.what();
    } catch (...) {
        qDebug() << "Unknown error when generating image.";
    }

    return fallbackImage(realSize);
}
