/*
 * Copyright (C) 2015 Canonical, Ltd.
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

#include "albumarthandler.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <unity/util/ResourcePtr.h>

namespace unity {
namespace thumbnailer {
namespace service {

struct AlbumArtHandlerPrivate {
    const std::shared_ptr<Thumbnailer> thumbnailer;
    const QString artist;
    const QString album;
    const QSize requestedSize;

    AlbumArtHandlerPrivate(const std::shared_ptr<Thumbnailer> &thumbnailer,
                           const QString &artist,
                           const QString &album,
                           const QSize &requestedSize)
        : thumbnailer(thumbnailer), artist(artist), album(album),
          requestedSize(requestedSize) {
    }
};

AlbumArtHandler::AlbumArtHandler(const QDBusConnection &bus,
                                 const QDBusMessage &message,
                                 const std::shared_ptr<Thumbnailer> &thumbnailer,
                                 const QString &artist,
                                 const QString &album,
                                 const QSize &requestedSize)
    : Handler(bus, message), p(new AlbumArtHandlerPrivate(thumbnailer, artist, album, requestedSize)) {
}

AlbumArtHandler::~AlbumArtHandler() {
}

QDBusUnixFileDescriptor AlbumArtHandler::check() {
    return QDBusUnixFileDescriptor();
}

void AlbumArtHandler::download() {
    downloadFinished();
}

QDBusUnixFileDescriptor AlbumArtHandler::create() {
    int size = thumbnail_size_from_qsize(p->requestedSize);
    std::string art_image = p->thumbnailer->get_album_art(
        p->artist.toStdString(), p->album.toStdString(), size);

    if (art_image.empty()) {
        throw std::runtime_error("AlbumArtHandler::create(): Could not get thumbnail for " +
                                 p->artist.toStdString() + ", " + p->album.toStdString());
    }

    return write_to_tmpfile(art_image);
}

}
}
}
