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
    const ThumbnailSize size;

    AlbumArtHandlerPrivate(const std::shared_ptr<Thumbnailer> &thumbnailer,
                           const QString &artist,
                           const QString &album,
                           ThumbnailSize size)
        : thumbnailer(thumbnailer), artist(artist), album(album), size(size) {
    }
};

AlbumArtHandler::AlbumArtHandler(const QDBusConnection &bus,
                                 const QDBusMessage &message,
                                 const std::shared_ptr<Thumbnailer> &thumbnailer,
                                 const QString &artist,
                                 const QString &album,
                                 ThumbnailSize size)
    : Handler(bus, message), p(new AlbumArtHandlerPrivate(thumbnailer, artist, album, size)) {
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
    std::string art_image = p->thumbnailer->get_album_art(
        p->artist.toStdString(), p->album.toStdString(), p->size, TN_REMOTE);

    if (art_image.empty()) {
        throw std::runtime_error("Could not get thumbnail");
    }

    return write_to_tmpfile(art_image);
}

}
}
}
