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

#include "thumbnailhandler.h"
#include <internal/raii.h>
#include <internal/safe_strerror.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <unity/util/ResourcePtr.h>

using namespace unity::thumbnailer::internal;

namespace unity {
namespace thumbnailer {
namespace service {

struct ThumbnailHandlerPrivate {
    const std::shared_ptr<Thumbnailer> thumbnailer;
    const QString filename;
    const QDBusUnixFileDescriptor filename_fd;
    const QSize requestedSize;
    std::unique_ptr<ThumbnailRequest> request;

    ThumbnailHandlerPrivate(const std::shared_ptr<Thumbnailer> &thumbnailer,
                            const QString &filename,
                            const QDBusUnixFileDescriptor &filename_fd,
                            const QSize &requestedSize)
        : thumbnailer(thumbnailer), filename(filename),
          filename_fd(filename_fd), requestedSize(requestedSize) {
    }
};

ThumbnailHandler::ThumbnailHandler(const QDBusConnection &bus,
                                   const QDBusMessage &message,
                                   const std::shared_ptr<Thumbnailer> &thumbnailer,
                                   const QString &filename,
                                   const QDBusUnixFileDescriptor &filename_fd,
                                   const QSize &requestedSize)
    : Handler(bus, message), p(new ThumbnailHandlerPrivate(thumbnailer, filename, filename_fd, requestedSize)) {
}

ThumbnailHandler::~ThumbnailHandler() {
}

QDBusUnixFileDescriptor ThumbnailHandler::check() {
    struct stat filename_stat, fd_stat;

    if (stat(p->filename.toUtf8(), &filename_stat) < 0) {
        throw std::runtime_error("ThumbnailHandler::create(): Could not stat " +
                                 p->filename.toStdString() + ": " + safe_strerror(errno));
    }
    if (fstat(p->filename_fd.fileDescriptor(), &fd_stat) < 0) {
        throw std::runtime_error("ThumbnailHandler::create(): Could not stat file descriptor: " + safe_strerror(errno));
    }
    if (filename_stat.st_dev != fd_stat.st_dev ||
        filename_stat.st_ino != fd_stat.st_ino) {
        throw std::runtime_error("ThumbnailHandler::create(): " + p->filename.toStdString()
                                 + " refers to a different file than the file descriptor");
    }

    p->request = p->thumbnailer->get_thumbnail(
        p->filename.toStdString(), p->requestedSize);
    std::string art_image = p->request->thumbnail();

    if (art_image.empty()) {
        return QDBusUnixFileDescriptor();
    }
    return write_to_tmpfile(art_image);
}

void ThumbnailHandler::download() {
    connect(p->request.get(), &ThumbnailRequest::downloadFinished,
            this, &ThumbnailHandler::downloadFinished);
    p->request->download();
}

QDBusUnixFileDescriptor ThumbnailHandler::create() {
    std::string art_image = p->request->thumbnail();

    if (art_image.empty()) {
        throw std::runtime_error("ThumbnailHandler::create(): Could not get thumbnail for " + p->filename.toStdString());
    }

    // FIXME: check that the thumbnail was produced for fd_stat
    return write_to_tmpfile(art_image);
}

}
}
}
