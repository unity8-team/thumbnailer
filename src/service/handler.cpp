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

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <QtConcurrent>
#include <QFuture>
#include <QFutureWatcher>
#include <unity/util/ResourcePtr.h>

namespace {
const char ART_ERROR[] = "com.canonical.Thumbnailer.Error.Failed";
}

namespace unity {
namespace thumbnailer {
namespace service {

struct HandlerPrivate {
    QDBusConnection bus;
    const QDBusMessage message;

    QFutureWatcher<QDBusUnixFileDescriptor> checkWatcher;
    QFutureWatcher<QDBusUnixFileDescriptor> createWatcher;

    HandlerPrivate(const QDBusConnection &bus, const QDBusMessage &message)
        : bus(bus), message(message) {
    }
};

Handler::Handler(const QDBusConnection &bus, const QDBusMessage &message)
    : p(new HandlerPrivate(bus, message)) {
    connect(&p->checkWatcher, &QFutureWatcher<QDBusUnixFileDescriptor>::finished,
            this, &Handler::checkFinished);
    connect(&p->createWatcher, &QFutureWatcher<QDBusUnixFileDescriptor>::finished,
            this, &Handler::createFinished);
}

Handler::~Handler() {
}

void Handler::begin() {
    p->checkWatcher.setFuture(QtConcurrent::run(this, &Handler::check));
}

void Handler::checkFinished() {
    QDBusUnixFileDescriptor unix_fd;
    try {
        unix_fd = p->checkWatcher.result();
    } catch (const std::exception &e) {
        sendError(e.what());
        return;
    }
    // Did we find a valid thumbnail in the cache?
    if (unix_fd.isValid()) {
        sendThumbnail(unix_fd);
    } else {
        // otherwise move on to the download phase.
        download();
    }
}

void Handler::downloadFinished() {
    p->createWatcher.setFuture(QtConcurrent::run(this, &Handler::create));
}

void Handler::createFinished() {
    QDBusUnixFileDescriptor unix_fd;
    try {
        unix_fd = p->createWatcher.result();
    } catch (const std::exception &e) {
        sendError(e.what());
        return;
    }
    // Did we find a valid thumbnail in the cache?
    if (unix_fd.isValid()) {
        sendThumbnail(unix_fd);
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
    if (fd < 0) {
        std::string err = "cannot create tmpfile in " + dir + ": " + strerror(errno);
        throw std::runtime_error(err);
    }
    FdPtr fd_ptr(fd, ::close);
    auto rc = write(fd_ptr.get(), &image[0], image.size());
    if (rc == -1)
    {
        std::string err = "cannot write image data in " + dir + ": " + strerror(errno);
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
