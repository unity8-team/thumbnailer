/*
 * Copyright (C) 2014 Canonical, Ltd.
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

#include "dbusinterface.h"

#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDebug>
#include <QRunnable>
#include <QThreadPool>

#include <thumbnailer.h>

using namespace std;

static const char ART_ERROR[] = "com.canonical.MediaScanner2.Error.Failed";

struct DBusInterfacePrivate {
    std::shared_ptr<Thumbnailer> thumbnailer = std::make_shared<Thumbnailer>();
    QThreadPool pool;
};

namespace {

ThumbnailSize desiredSizeFromString(const QString &size)
{
    if (size == "small") {
        return TN_SIZE_SMALL;
    } else if (size == "large") {
        return TN_SIZE_LARGE;
    } else if (size == "xlarge") {
        return TN_SIZE_XLARGE;
    } else if (size == "original") {
        return TN_SIZE_ORIGINAL;
    }
    std::string error("Unknown size: ");
    error += size.toStdString();
    throw std::logic_error(error);
}

class Task final : public QRunnable {
public:
    Task(std::function<void()> task) : task(task) {}
    virtual void run() override {
        task();
    }
private:
    std::function<void()> task;
};
}

DBusInterface::DBusInterface(QObject *parent)
    : QObject(parent), p(new DBusInterfacePrivate) {
}

DBusInterface::~DBusInterface() {
}

QDBusUnixFileDescriptor DBusInterface::GetAlbumArt(const QString &artist, const QString &album, const QString &desiredSize) {
    qDebug() << "Look up cover art for" << artist << "/" << album << "at size" << desiredSize;

    ThumbnailSize size;
    try {
        size = desiredSizeFromString(desiredSize);
    } catch (const std::logic_error& e) {
        sendErrorReply(ART_ERROR, e.what());
        return QDBusUnixFileDescriptor();
    }

    setDelayedReply(true);
    auto thumbnailer = p->thumbnailer;
    auto bus = connection();
    auto msg = message();
    p->pool.start(new Task([thumbnailer, artist, album, size, bus, msg]() {
        std::string art;
        try {
            art = thumbnailer->get_album_art(
                artist.toStdString(), album.toStdString(), size, TN_REMOTE);
        } catch (const std::exception &e) {
            bus.send(msg.createErrorReply(ART_ERROR, e.what()));
            return;
        }

        if (art.empty()) {
            bus.send(msg.createErrorReply(ART_ERROR, "Could not get thumbnail"));
            return;
        }
        int fd = open(art.c_str(), O_RDONLY);
        if (fd < 0) {
            bus.send(msg.createErrorReply(ART_ERROR, strerror(errno)));
            return;
        }

        QDBusUnixFileDescriptor unix_fd(fd);
        close(fd);

        bus.send(msg.createReply(QVariant::fromValue(unix_fd)));
    }));

    return QDBusUnixFileDescriptor();
}

QDBusUnixFileDescriptor DBusInterface::GetArtistArt(const QString &artist, const QString &album, const QString &desiredSize) {
    qDebug() << "Look up artist art for" << artist << "/" << album << "at size" << desiredSize;

    ThumbnailSize size;
    try {
        size = desiredSizeFromString(desiredSize);
    } catch (const std::logic_error& e) {
        sendErrorReply(ART_ERROR, e.what());
        return QDBusUnixFileDescriptor();
    }

    setDelayedReply(true);
    auto thumbnailer = p->thumbnailer;
    auto bus = connection();
    auto msg = message();
    p->pool.start(new Task([thumbnailer, artist, album, size, bus, msg]() {
        std::string art;
        try {
            art = thumbnailer->get_artist_art(
                artist.toStdString(), album.toStdString(), size, TN_REMOTE);
        } catch (const std::exception &e) {
            bus.send(msg.createErrorReply(ART_ERROR, e.what()));
            return;
        }

        if (art.empty()) {
            bus.send(msg.createErrorReply(ART_ERROR, "Could not get thumbnail"));
            return;
        }
        int fd = open(art.c_str(), O_RDONLY);
        if (fd < 0) {
            bus.send(msg.createErrorReply(ART_ERROR, strerror(errno)));
            return;
        }

        QDBusUnixFileDescriptor unix_fd(fd);
        close(fd);

        bus.send(msg.createReply(QVariant::fromValue(unix_fd)));
    }));

    return QDBusUnixFileDescriptor();
}

QDBusUnixFileDescriptor DBusInterface::GetThumbnail(const QString &filename, const QDBusUnixFileDescriptor &filename_fd, const QString &desiredSize) {
    qDebug() << "Look thumbnail for" << filename << "at size" << desiredSize;

    ThumbnailSize size;
    try {
        size = desiredSizeFromString(desiredSize);
    } catch (const std::logic_error& e) {
        sendErrorReply(ART_ERROR, e.what());
        return QDBusUnixFileDescriptor();
    }

    struct stat filename_stat, fd_stat;
    if (stat(filename.toUtf8(), &filename_stat) < 0) {
        sendErrorReply(ART_ERROR, "Could not stat file");
        return QDBusUnixFileDescriptor();
    }
    if (fstat(filename_fd.fileDescriptor(), &fd_stat) < 0) {
        sendErrorReply(ART_ERROR, "Could not stat file descriptor");
        return QDBusUnixFileDescriptor();
    }
    if (filename_stat.st_dev != fd_stat.st_dev ||
        filename_stat.st_ino != fd_stat.st_ino) {
        sendErrorReply(ART_ERROR, "filename refers to a different file to the file descriptor");
        return QDBusUnixFileDescriptor();
    }

    setDelayedReply(true);
    auto thumbnailer = p->thumbnailer;
    auto bus = connection();
    auto msg = message();
    p->pool.start(new Task([=]() {
        std::string art;
        try {
            art = thumbnailer->get_thumbnail(
                filename.toStdString(), size, TN_REMOTE);
        } catch (const std::exception &e) {
            bus.send(msg.createErrorReply(ART_ERROR, e.what()));
            return;
        }

        if (art.empty()) {
            bus.send(msg.createErrorReply(ART_ERROR, "Could not get thumbnail"));
            return;
        }

        // FIXME: check that the thumbnail was produced for fd_stat
        int fd = open(art.c_str(), O_RDONLY);
        if (fd < 0) {
            bus.send(msg.createErrorReply(ART_ERROR, strerror(errno)));
            return;
        }

        QDBusUnixFileDescriptor unix_fd(fd);
        close(fd);

        bus.send(msg.createReply(QVariant::fromValue(unix_fd)));
    }));

    return QDBusUnixFileDescriptor();
}
