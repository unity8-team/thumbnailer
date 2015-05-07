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

#include "artistarthandler.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <unity/util/ResourcePtr.h>

namespace unity {
namespace thumbnailer {
namespace service {

struct ArtistArtHandlerPrivate {
    const std::shared_ptr<Thumbnailer> thumbnailer;
    const QString artist;
    const QString album;
    const QSize requestedSize;
    std::unique_ptr<ThumbnailRequest> request;

    ArtistArtHandlerPrivate(const std::shared_ptr<Thumbnailer> &thumbnailer,
                            const QString &artist,
                            const QString &album,
                            const QSize &requestedSize)
        : thumbnailer(thumbnailer), artist(artist), album(album),
          requestedSize(requestedSize) {
    }
};

ArtistArtHandler::ArtistArtHandler(const QDBusConnection &bus,
                                   const QDBusMessage &message,
                                   const std::shared_ptr<Thumbnailer> &thumbnailer,
                                   std::shared_ptr<QThreadPool> do_check_pool,
                                   std::shared_ptr<QThreadPool> do_create_pool,
                                   const QString &artist,
                                   const QString &album,
                                   const QSize &requestedSize)
    : Handler(bus, message,  do_check_pool, do_create_pool),
      p(new ArtistArtHandlerPrivate(thumbnailer, artist, album, requestedSize)) {
}

ArtistArtHandler::~ArtistArtHandler() {
}

QDBusUnixFileDescriptor ArtistArtHandler::check() {
    p->request = p->thumbnailer->get_artist_art(
        p->artist.toStdString(), p->album.toStdString(), p->requestedSize);
    std::string art_image = p->request->thumbnail();

    if (art_image.empty()) {
        return QDBusUnixFileDescriptor();
    }
    return write_to_tmpfile(art_image);
}

void ArtistArtHandler::download() {
    connect(p->request.get(), &ThumbnailRequest::downloadFinished,
            this, &ArtistArtHandler::downloadFinished);
    p->request->download();
}

QDBusUnixFileDescriptor ArtistArtHandler::create() {
    std::string art_image = p->request->thumbnail();

    if (art_image.empty()) {
        throw std::runtime_error("ArtistArtHandler::create() Could not get thumbnail for " + p->artist.toStdString());
    }

    return write_to_tmpfile(art_image);
}

}
}
}
