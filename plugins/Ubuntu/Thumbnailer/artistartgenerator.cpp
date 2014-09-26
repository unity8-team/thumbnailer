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
 *          Pawel Stolowski <pawel.stolowski@canonical.com>
*/

#include "artistartgenerator.h"
#include "artgeneratorcommon.h"
#include <stdexcept>
#include <QDebug>
#include <QFile>
#include <QUrlQuery>
#include <QDBusUnixFileDescriptor>
#include <QDBusReply>

static const char DEFAULT_ARTIST_ART[] = "/usr/share/thumbnailer/icons/album_missing.png";

static const char BUS_NAME[] = "com.canonical.Thumbnailer";
static const char BUS_PATH[] = "/com/canonical/Thumbnailer";
static const char THUMBNAILER_IFACE[] = "com.canonical.Thumbnailer";
static const char GET_ARTIST_ART[] = "GetArtistArt";

ArtistArtGenerator::ArtistArtGenerator()
    : QQuickImageProvider(QQuickImageProvider::Image, QQmlImageProviderBase::ForceAsynchronousImageLoading),
      iface(BUS_NAME, BUS_PATH, THUMBNAILER_IFACE) {
}

static QImage fallbackImage(QSize *realSize) {
    QImage fallback;
    fallback.load(DEFAULT_ARTIST_ART);
    *realSize = fallback.size();
    return fallback;
}

QImage ArtistArtGenerator::requestImage(const QString &id, QSize *realSize,
        const QSize &requestedSize) {
    QUrlQuery query(id);
    if (!query.hasQueryItem("artist") || !query.hasQueryItem("album")) {
        qWarning() << "Invalid artistart uri:" << id;
        return fallbackImage(realSize);
    }

    const QString artist = query.queryItemValue("artist", QUrl::FullyDecoded);
    const QString album = query.queryItemValue("album", QUrl::FullyDecoded);

    QString desiredSize = sizeToDesiredSizeString(requestedSize);

    // perform dbus call
    QDBusReply<QDBusUnixFileDescriptor> reply = iface.call(
        GET_ARTIST_ART, artist, album, desiredSize);
    if (!reply.isValid()) {
        qWarning() << "D-Bus error: " << reply.error().message();
        return fallbackImage(realSize);
    }

    try {
        return imageFromFd(reply.value().fileDescriptor(), realSize);
    } catch (const std::exception &e) {
        qDebug() << "Artist art loader failed: " << e.what();
    } catch (...) {
        qDebug() << "Unknown error when generating image.";
    }

    return fallbackImage(realSize);
}
