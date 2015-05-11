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
    std::unique_ptr<ThumbnailRequest> request;

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
                                 std::shared_ptr<QThreadPool> check_pool,
                                 std::shared_ptr<QThreadPool> create_pool,
                                 const QString &artist,
                                 const QString &album,
                                 const QSize &requestedSize)
    : Handler(bus, message,  check_pool, create_pool),
      p(new AlbumArtHandlerPrivate(thumbnailer, artist, album, requestedSize)) {
}

AlbumArtHandler::~AlbumArtHandler() {
}

QDBusUnixFileDescriptor AlbumArtHandler::check() {
    p->request = p->thumbnailer->get_album_art(
        p->artist.toStdString(), p->album.toStdString(), p->requestedSize);
    std::string art_image = p->request->thumbnail();

    if (art_image.empty()) {
        return QDBusUnixFileDescriptor();
    }
    return write_to_tmpfile(art_image);
}

void AlbumArtHandler::download() {
    connect(p->request.get(), &ThumbnailRequest::downloadFinished,
            this, &AlbumArtHandler::downloadFinished);
    p->request->download();
}

QDBusUnixFileDescriptor AlbumArtHandler::create() {
    std::string art_image = p->request->thumbnail();

    if (art_image.empty()) {
        throw std::runtime_error("AlbumArtHandler::create(): Could not get thumbnail for " +
                                 p->artist.toStdString() + ", " + p->album.toStdString());
    }

    return write_to_tmpfile(art_image);
}

}
}
}
