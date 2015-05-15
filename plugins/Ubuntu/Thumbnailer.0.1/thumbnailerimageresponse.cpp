/*
 * Copyright 2015 Canonical Ltd.
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
 * Authors: Xavi Garcia <xavi.garcia.mena@canonical.com>
*/

#include <thumbnailerimageresponse.h>
#include <artgeneratorcommon.h>

#include <QCoreApplication>
#include <QDBusPendingReply>
#include <QDBusPendingCallWatcher>
#include <QDBusUnixFileDescriptor>
#include <QDBusReply>
#include <QEvent>
#include <QDebug>
#include <QMimeDatabase>

namespace unity
{

namespace thumbnailer
{

namespace qml
{
class FinishResponseEvent : public QEvent
{
public:
    FinishResponseEvent()
        : QEvent(QEvent::User){};
};

ThumbnailerImageResponse::ThumbnailerImageResponse(ResponseType type,
                                                   QString const& id,
                                                   QSize const& requested_size,
                                                   QString const& default_video_image,
                                                   QString const& default_audio_image)
    : id_(id)
    , requested_size_(requested_size)
    , texture_(nullptr)
    , default_video_image_(default_video_image)
    , default_audio_image_(default_audio_image)
    , type_(type)
{
}

QQuickTextureFactory* ThumbnailerImageResponse::textureFactory() const
{
    return texture_.get();
}

void ThumbnailerImageResponse::set_default_image()
{
    QImage result;

    switch(type_)
    {
    case Download:
        // we are only downloading images for audio files
        result.load(default_audio_image_);
        break;
    case Thumbnail:
        result.load(return_default_image_based_on_mime());
        break;
    default:
        abort();  // LCOV_EXCL_LINE  // Impossible
    }
    texture_.reset(QQuickTextureFactory::textureFactoryForImage(result));
}

void ThumbnailerImageResponse::finish_later_with_default_image()
{
    QCoreApplication::postEvent(this, new FinishResponseEvent());
}

bool ThumbnailerImageResponse::event(QEvent* event)
{
    if (event->type() == QEvent::User)
    {
        set_default_image();
        Q_EMIT finished();
        return true;
    }
    return false;
}

void ThumbnailerImageResponse::dbus_call_finished(QDBusPendingCallWatcher* watcher)
{
    QDBusPendingReply<QDBusUnixFileDescriptor> reply = *watcher;
    if (!reply.isValid())
    {
        qWarning() << "D-Bus error: " << reply.error().message();
        set_default_image();
        Q_EMIT finished();
        return;
    }

    try
    {
        QSize realSize;
        QImage image = imageFromFd(reply.value().fileDescriptor(), &realSize, requested_size_);
        texture_.reset(QQuickTextureFactory::textureFactoryForImage(image));
        Q_EMIT finished();
        return;
    }
    catch (const std::exception& e)
    {
        qWarning() << "Album art loader failed: " << e.what();
    }
    catch (...)
    {
        qWarning() << "Unknown error when generating image.";
    }

    set_default_image();
    Q_EMIT finished();
}

QString ThumbnailerImageResponse::return_default_image_based_on_mime()
{
    QMimeDatabase db;
    QMimeType mime = db.mimeTypeForFile(id_);

    if (mime.name().contains("audio"))
    {
        return default_audio_image_;
    }
    else if (mime.name().contains("video"))
    {
        return default_video_image_;
    }
    return default_audio_image_;
}

}  // namespace qml

}  // namespace thumbnailer

}  // namespace unity
