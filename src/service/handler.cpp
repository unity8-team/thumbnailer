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

#include "handler.h"
#include <internal/safe_strerror.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <QtConcurrent>
#include <QFuture>
#include <QFutureWatcher>
#include <QDebug>
#include <unity/util/ResourcePtr.h>

using namespace unity::thumbnailer::internal;

namespace {
const char ART_ERROR[] = "com.canonical.Thumbnailer.Error.Failed";

struct FdOrError {
    QDBusUnixFileDescriptor fd;
    QString error;
};
}

namespace unity {
namespace thumbnailer {
namespace service {

struct HandlerPrivate {
    QDBusConnection bus;
    const QDBusMessage message;

    bool cancelled = false;
    QFutureWatcher<FdOrError> checkWatcher;
    QFutureWatcher<FdOrError> createWatcher;

    HandlerPrivate(const QDBusConnection &bus, const QDBusMessage &message)
        : bus(bus), message(message) {
    }
};

Handler::Handler(const QDBusConnection &bus, const QDBusMessage &message)
    : p(new HandlerPrivate(bus, message)) {
    connect(&p->checkWatcher, &QFutureWatcher<FdOrError>::finished,
            this, &Handler::checkFinished);
    connect(&p->createWatcher, &QFutureWatcher<FdOrError>::finished,
            this, &Handler::createFinished);
}

Handler::~Handler() {
    p->cancelled = true;
    // ensure that jobs occurring in the thread pool complete.
    p->checkWatcher.waitForFinished();
    p->createWatcher.waitForFinished();
    qDebug() << "Handler" << this << "destroyed";
}

void Handler::begin() {
    auto do_check = [this]() -> FdOrError {
        try {
            return FdOrError{check(), nullptr};
        } catch (const std::exception &e) {
            return FdOrError{QDBusUnixFileDescriptor(), e.what()};
        }
    };
    p->checkWatcher.setFuture(QtConcurrent::run(do_check));
}

void Handler::checkFinished() {
    if (p->cancelled)
        return;

    FdOrError fd_error;
    try {
        fd_error = p->checkWatcher.result();
    } catch (const std::exception &e) {
        sendError(e.what());
        return;
    }
    if (!fd_error.error.isNull()) {
        sendError(fd_error.error);
        return;
    }
    // Did we find a valid thumbnail in the cache?
    if (fd_error.fd.isValid()) {
        sendThumbnail(fd_error.fd);
    } else {
        // otherwise move on to the download phase.
        download();
    }
}

void Handler::downloadFinished() {
    if (p->cancelled)
        return;

    auto do_create = [this]() -> FdOrError {
        try {
            return FdOrError{create(), nullptr};
        } catch (const std::exception &e) {
            return FdOrError{QDBusUnixFileDescriptor(), e.what()};
        }
    };
    p->createWatcher.setFuture(QtConcurrent::run(do_create));
}

void Handler::createFinished() {
    if (p->cancelled)
        return;

    FdOrError fd_error;
    try {
        fd_error = p->createWatcher.result();
    } catch (const std::exception &e) {
        sendError(e.what());
        return;
    }
    if (!fd_error.error.isEmpty()) {
        sendError(fd_error.error);
        return;
    }
    // Did we find a valid thumbnail in the cache?
    if (fd_error.fd.isValid()) {
        sendThumbnail(fd_error.fd);
    } else {
        // otherwise move on to the download phase.
        download();
    }
}

void Handler::sendThumbnail(const QDBusUnixFileDescriptor &unix_fd) {
    p->bus.send(p->message.createReply(QVariant::fromValue(unix_fd)));
    Q_EMIT finished();
}

void Handler::sendError(const QString &error) {
    p->bus.send(p->message.createErrorReply(ART_ERROR, error));
    Q_EMIT finished();
}

int thumbnail_size_from_qsize(const QSize &size)
{
    if (!size.isValid()) {
        // If an invalid size is passed to the QQuickImageProvider,
        // then we don't know what size is expected.  In this case,
        // return the unscaled original.
        return 0;
    }
    return std::max(size.width(), size.height());
}

QDBusUnixFileDescriptor write_to_tmpfile(std::string const& image)
{

    typedef unity::util::ResourcePtr<int, decltype(&::close)> FdPtr;

    static auto find_tmpdir = []
    {
        char const* dirp = getenv("TMPDIR");
        std::string dir = dirp ? dirp : "/tmp";
        return dir;
    };
    static std::string dir = find_tmpdir();

    int fd = open(dir.c_str(), O_TMPFILE | O_RDWR);
    // Different kernel verions return different errno if they don't recognize O_TMPFILE,
    // so we check for any failure here. If it is a real failure, mkstemp will fail too.
    if (fd < 0)
    {
        // We are running on an old kernel without O_TMPFILE support:
        // the flag has been ignored, and treated as an attempt to
        // open /tmp.
        //
        // As a fallback, use mkstemp() and unlink the resulting file.
        std::string tmpfile = dir + "/thumbnail.XXXXXX";
        fd = mkstemp(const_cast<char*>(tmpfile.data()));
        if (fd >= 0)
        {
            unlink(tmpfile.data());
        }
    }
    if (fd < 0) {
        std::string err = "cannot create tmpfile in " + dir + ": " + safe_strerror(errno);
        throw std::runtime_error(err);
    }
    FdPtr fd_ptr(fd, ::close);
    auto rc = write(fd_ptr.get(), &image[0], image.size());
    if (rc == -1)
    {
        std::string err = "cannot write image data in " + dir + ": " + safe_strerror(errno);
        throw std::runtime_error(err);
    }
    else if (std::string::size_type(rc) != image.size())
    {
        std::string err = "short write for image data in " + dir +
            "(requested = " + std::to_string(image.size()) + ", actual = " + std::to_string(rc) + ")";
        throw std::runtime_error(err);
    }
    lseek(fd, SEEK_SET, 0);  // No error check needed, can't fail.

    return QDBusUnixFileDescriptor(fd_ptr.get());
}

}
}
}
