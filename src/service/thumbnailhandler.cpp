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
    return QDBusUnixFileDescriptor();
}

void ThumbnailHandler::download() {
    downloadFinished();
}

QDBusUnixFileDescriptor ThumbnailHandler::create() {
    typedef unity::util::ResourcePtr<int, decltype(&::close)> FdPtr;
    struct stat filename_stat, fd_stat;

    if (stat(p->filename.toUtf8(), &filename_stat) < 0) {
        throw std::runtime_error("Could not stat file");
    }
    if (fstat(p->filename_fd.fileDescriptor(), &fd_stat) < 0) {
        throw std::runtime_error("Could not stat file descriptor");
    }
    if (filename_stat.st_dev != fd_stat.st_dev ||
        filename_stat.st_ino != fd_stat.st_ino) {
        throw std::runtime_error("filename refers to a different file to the file descriptor");
    }

    ThumbnailSize size = thumbnail_size_from_qsize(p->requestedSize);
    std::string art = p->thumbnailer->get_thumbnail(
        p->filename.toStdString(), size, TN_REMOTE);

    if (art.empty()) {
        throw std::runtime_error("Could not get thumbnail");
    }

    // FIXME: check that the thumbnail was produced for fd_stat
    FdPtr fd(open(art.c_str(), O_RDONLY), ::close);
    if (fd.get() < 0) {
        throw std::runtime_error(safe_strerror(errno));
    }
    return QDBusUnixFileDescriptor(fd.get());
}

}
}
}
