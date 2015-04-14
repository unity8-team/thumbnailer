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

#include <thumbnailer.h>

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDebug>
#include <QRunnable>
#include <QThreadPool>
#include <unity/util/ResourcePtr.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

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

namespace
{

typedef unity::util::ResourcePtr<int, decltype(&::close)> FdPtr;

FdPtr write_to_tmpfile(string const& image)
{
    static auto find_tmpdir = []
    {
        char const* dirp = getenv("TMPDIR");
        string dir = dirp ? dirp : "/tmp";
        return dir;
    };
    static string dir = find_tmpdir();

    int fd = open(dir.c_str(), O_TMPFILE | O_RDWR);
    if (fd < 0) {
        string err = "cannot create tmpfile in " + dir + ": " + strerror(errno);
        throw runtime_error(err);
    }
    FdPtr fd_ptr(fd, ::close);
    auto rc = write(fd_ptr.get(), &image[0], image.size());
    if (rc == -1)
    {
        string err = "cannot write image data in " + dir + ": " + strerror(errno);
        throw runtime_error(err);
    }
    else if (string::size_type(rc) != image.size())
    {
        string err = "short write for image data in " + dir +
                     "(requested = " + to_string(image.size()) + ", actual = " + to_string(rc) + ")";
        throw runtime_error(err);
    }
    lseek(fd, SEEK_SET, 0);  // No error check needed, can't fail.

    return fd_ptr;
}

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
        std::string art_image;
        try {
            art_image = thumbnailer->get_album_art(
                artist.toStdString(), album.toStdString(), size, TN_REMOTE);
        } catch (const std::exception &e) {
            bus.send(msg.createErrorReply(ART_ERROR, e.what()));
            return;
        }

        if (art_image.empty()) {
            bus.send(msg.createErrorReply(ART_ERROR, "Could not get thumbnail"));
            return;
        }

        try
        {
            auto fd_ptr = write_to_tmpfile(art_image);
            QDBusUnixFileDescriptor unix_fd(fd_ptr.get());
            bus.send(msg.createReply(QVariant::fromValue(unix_fd)));
        }
        catch (runtime_error const& e)
        {
            string err = string("GetAlbumArt(): ") + e.what();
            bus.send(msg.createErrorReply(ART_ERROR, err.c_str()));
        }
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
        std::string art_image;
        try {
            art_image = thumbnailer->get_artist_art(
                artist.toStdString(), album.toStdString(), size, TN_REMOTE);
        } catch (const std::exception &e) {
            bus.send(msg.createErrorReply(ART_ERROR, e.what()));
            return;
        }

        if (art_image.empty()) {
            bus.send(msg.createErrorReply(ART_ERROR, "Could not get thumbnail"));
            return;
        }

        try
        {
            auto fd_ptr = write_to_tmpfile(art_image);
            QDBusUnixFileDescriptor unix_fd(fd_ptr.get());
            bus.send(msg.createReply(QVariant::fromValue(unix_fd)));
        }
        catch (runtime_error const& e)
        {
            string err = string("GetArtistArt(): ") + e.what();
            bus.send(msg.createErrorReply(ART_ERROR, err.c_str()));
        }
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
