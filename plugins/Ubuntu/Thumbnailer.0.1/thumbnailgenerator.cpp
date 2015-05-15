/*
 * Copyright 2013 Canonical Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Jussi Pakkanen <jussi.pakkanen@canonical.com>
*/

#include "thumbnailgenerator.h"
#include "artgeneratorcommon.h"

#include <stdexcept>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <QDebug>
#include <QMimeDatabase>
#include <QUrl>
#include <QDBusUnixFileDescriptor>
#include <QDBusReply>

namespace
{
const char* DEFAULT_VIDEO_ART = "/usr/share/thumbnailer/icons/video_missing.png";
const char* DEFAULT_ALBUM_ART = "/usr/share/thumbnailer/icons/album_missing.png";

const char BUS_NAME[] = "com.canonical.Thumbnailer";
const char BUS_PATH[] = "/com/canonical/Thumbnailer";
}

namespace unity
{
namespace thumbnailer
{
namespace qml
{

ThumbnailGenerator::ThumbnailGenerator()
    : QQuickImageProvider(QQuickImageProvider::Image, QQmlImageProviderBase::ForceAsynchronousImageLoading)
{
}

QImage ThumbnailGenerator::requestImage(const QString& id, QSize* realSize, const QSize& requestedSize)
{
    /* Allow appending a query string (e.g. ?something=timestamp)
     * to the id and then ignore it.
     * This is workaround to force reloading a thumbnail when it has
     * the same file name on disk but we know the content has changed.
     * It is necessary because in such a situation the QML image cache
     * will kick in and this ImageProvider will never get called.
     * The only "solution" is setting Image.cache = false, but in some
     * cases we don't want to do that for performance reasons, so this
     * is the only way around the issue for now. */
    QString src_path = QUrl(id).path();
    int fd = open(src_path.toUtf8().constData(), O_RDONLY | O_CLOEXEC);
    if (fd < 0)
    {
        qDebug() << "Thumbnail generator failed: " << strerror(errno);
        return getFallbackImage(id, realSize, requestedSize);
    }
    QDBusUnixFileDescriptor unix_fd(fd);
    close(fd);

    if (!connection)
    {
        // Create them here and not them on the constrcutor so they belong to the proper thread
        connection.reset(new QDBusConnection(
            QDBusConnection::connectToBus(QDBusConnection::SessionBus, "thumbnail_generator_dbus_connection")));
        iface.reset(new ThumbnailerInterface(BUS_NAME, BUS_PATH, *connection));
    }

    auto reply = iface->GetThumbnail(src_path, unix_fd, requestedSize);
    reply.waitForFinished();
    if (!reply.isValid())
    {
        qWarning() << "D-Bus error: " << reply.error().message();
        return getFallbackImage(id, realSize, requestedSize);
    }

    try
    {
        return imageFromFd(reply.value().fileDescriptor(), realSize, requestedSize);
    }
    catch (const std::exception& e)
    {
        qWarning() << "Album art loader failed: " << e.what();
    }
    catch (...)
    {
        qWarning() << "Unknown error when generating image.";
    }

    return getFallbackImage(id, realSize, requestedSize);
}

QImage ThumbnailGenerator::getFallbackImage(const QString& id, QSize* size, const QSize& requestedSize)
{
    Q_UNUSED(requestedSize);
    QMimeDatabase db;
    QMimeType mime = db.mimeTypeForFile(id);
    QImage result;
    if (mime.name().contains("audio"))
    {
        result.load(DEFAULT_ALBUM_ART);
    }
    else if (mime.name().contains("video"))
    {
        result.load(DEFAULT_VIDEO_ART);
    }
    *size = result.size();
    return result;
}
}
}
}
