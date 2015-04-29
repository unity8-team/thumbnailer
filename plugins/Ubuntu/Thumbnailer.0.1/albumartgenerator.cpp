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

namespace {
const char DEFAULT_ALBUM_ART[] = "/usr/share/thumbnailer/icons/album_missing.png";

const char BUS_NAME[] = "com.canonical.Thumbnailer";
const char BUS_PATH[] = "/com/canonical/Thumbnailer";

QImage fallbackImage(QSize *realSize) {
    QImage fallback;
    fallback.load(DEFAULT_ALBUM_ART);
    *realSize = fallback.size();
    return fallback;
}
}

namespace unity {
namespace thumbnailer {
namespace qml {

AlbumArtGenerator::AlbumArtGenerator()
    : QQuickImageProvider(QQuickImageProvider::Image, QQmlImageProviderBase::ForceAsynchronousImageLoading)
{
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
        connection.reset(new QDBusConnection(QDBusConnection::connectToBus(QDBusConnection::SessionBus, "album_art_generator_dbus_connection")));
        iface.reset(new ThumbnailerInterface(BUS_NAME, BUS_PATH, *connection));
    }

    const QString artist = query.queryItemValue("artist", QUrl::FullyDecoded);
    const QString album = query.queryItemValue("album", QUrl::FullyDecoded);

    // perform dbus call
    auto reply = iface->GetAlbumArt(artist, album, requestedSize);
    reply.waitForFinished();
    if (!reply.isValid()) {
        qWarning() << "D-Bus error: " << reply.error().message();
        return fallbackImage(realSize);
    }

    try {
        return imageFromFd(reply.value().fileDescriptor(),
                           realSize, requestedSize);
    } catch (const std::exception &e) {
        qWarning() << "Album art loader failed: " << e.what();
    } catch (...) {
        qWarning() << "Unknown error when generating image.";
    }

    return fallbackImage(realSize);
}

}
}
}
