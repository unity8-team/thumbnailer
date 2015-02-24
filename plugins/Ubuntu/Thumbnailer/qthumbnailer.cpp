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
 * Authors: Florian Boucault <florian.boucault@canonical.com>
*/

#include "qthumbnailer.h"
#include <stdexcept>
#include <QtCore/QDebug>

/*!
  \qmltype Thumbnailer
  \instantiates QThumbnailer
  \inqmlmodule Ubuntu.Thumbnailer 0.1
  \ingroup ubuntu
  \brief Thumbnailer provides a way to load thumbnails of any media (image, video, etc.)

  When an image representation of a media, e.g. a video, is needed a thumbnail can be
  generated and retrieved by Thumbnailer. Once generated a thumbnail is cached on
  disk and reused when needed.

  Thumbnails generated have a size always greater or equal to \l size.

  In the following example a thumbnail of a media located at 'pathToMediaFile' is
  generated and then loaded by a standard QML Image object. Its size is set so that
  it matches exactly the size required by the Image as to minimize the computation
  and memory used while still looking as good as possible.

  \qml
  import QtQuick 2.0
  import Ubuntu.Thumbnailer 0.1

  Item {
      Thumbnailer {
          id: thumbnailer
          source: pathToMediaFile
          size: image.sourceSize
      }

      Image {
          id: image

          fillMode: Image.PreserveAspectFit
          asynchronous: true
          source: thumbnailer.thumbnail
          sourceSize {
              width: image.width
              height: image.height
          }
      }
  }
  \endqml

*/

QThumbnailer::QThumbnailer(QObject* parent) :
    QObject(parent),
    m_componentCompleted(false),
    m_source(""),
    m_thumbnail(""),
    m_thumbnailsize(TN_SIZE_SMALL),
    m_currentTask(NULL)
{
}

QThumbnailer::~QThumbnailer()
{
    cancelUpdateThumbnail();
}

void QThumbnailer::classBegin()
{
}

void QThumbnailer::componentComplete()
{
    m_componentCompleted = true;
    updateThumbnail();
}

/*!
  \qmlproperty url Thumbnailer::source

  URL of the media to be thumbnailed. Once set \l thumbnail will eventually
  hold the URL of the thumbnail.
 */
QUrl QThumbnailer::source() const
{
    return m_source;
}

void QThumbnailer::setSource(QUrl source)
{
    if (source != m_source) {
        m_source = source;
        Q_EMIT sourceChanged();
        updateThumbnail();
    }
}

/*!
  \qmlproperty size Thumbnailer::size

  Size requested for the thumbnail. The resulting thumbnail's size will be
  at least \l size.

  Warning: the thumbnail's size can be much greater than \l size.
  */
QSize QThumbnailer::size() const
{
    return m_size;
}

ThumbnailSize thumbnailSizeFromSize(QSize size)
{
    const int xlarge_cutoff = 512;
    const int large_cutoff = 256;
    const int small_cutoff = 128;
    if(size.width() > xlarge_cutoff || size.height() > xlarge_cutoff) {
        return TN_SIZE_ORIGINAL;
    } else if(size.width() > large_cutoff || size.height() > large_cutoff) {
        return TN_SIZE_XLARGE;
    } else if(size.width() > small_cutoff || size.height() > small_cutoff) {
        return TN_SIZE_LARGE;
    } else {
        return TN_SIZE_SMALL;
    }
}

void QThumbnailer::setSize(QSize size)
{
    if (size != m_size) {
        m_size = size;
        m_thumbnailsize = thumbnailSizeFromSize(m_size);
        Q_EMIT sizeChanged();
        updateThumbnail();
    }
}

/*!
  \qmlproperty url Thumbnailer::thumbnail

  URL of the thumbnail generated once \l source and \l size are set.
  */
QUrl QThumbnailer::thumbnail() const
{
    return m_thumbnail;
}

void QThumbnailer::setThumbnail(QString thumbnail)
{
    if (thumbnail.isEmpty()) {
        m_thumbnail = QUrl();
    } else {
        m_thumbnail = QUrl::fromLocalFile(thumbnail);
    }
    Q_EMIT thumbnailChanged();
}

void QThumbnailer::updateThumbnail()
{
    if (!m_componentCompleted) {
        return;
    }

    cancelUpdateThumbnail();

    if (m_size.isEmpty()) {
        return;
    }

    bool slowUpdate = s_thumbnailer.thumbnail_needs_generation(m_source.path().toStdString(),
                                                               m_thumbnailsize);
    if (!slowUpdate) {
        // if we know retrieving the thumbnail is fast because it is readily
        // available on disk, set it immediately
        QString thumbnail = thumbnailPathForMedia(m_source.path(), m_thumbnailsize);
        setThumbnail(thumbnail);
    } else {
        // otherwise enqueue the thumbnailing task that will then get processed
        // in a background thread and will end up setting the thumbnail URL
        ThumbnailTask* task = new ThumbnailTask;
        task->source = m_source;
        task->size = m_thumbnailsize;
        task->caller = this;
        m_currentTask = task;
        enqueueThumbnailTask(task);
    }
}

void QThumbnailer::cancelUpdateThumbnail()
{
    if (!m_currentTask.isNull()) {
        if (s_imageQueue.removeTask(m_currentTask.data()) ||
            s_videoQueue.removeTask(m_currentTask.data()) ) {
            delete m_currentTask;
            m_currentTask.clear();
        }
    }
}


/* Static methods implementation */

static const QString DEFAULT_VIDEO_ART("/usr/share/thumbnailer/icons/video_missing.png");
static const QString DEFAULT_ALBUM_ART("/usr/share/thumbnailer/icons/album_missing.png");

ThumbnailQueue QThumbnailer::s_videoQueue;
ThumbnailQueue QThumbnailer::s_imageQueue;
Thumbnailer QThumbnailer::s_thumbnailer;
QMimeDatabase QThumbnailer::s_mimeDatabase;

QString QThumbnailer::thumbnailPathForMedia(QString mediaPath, ThumbnailSize size)
{
    QString thumbnailPath;
    try {
        thumbnailPath = QString::fromStdString(s_thumbnailer.get_thumbnail(mediaPath.toStdString(),
                                               (ThumbnailSize)size));
    } catch(std::runtime_error &e) {
        qWarning() << QString("Thumbnail retrieval for %1 failed:").arg(mediaPath).toLocal8Bit().constData() << e.what();
    }

    if(thumbnailPath.isEmpty()) {
        QMimeType mime = s_mimeDatabase.mimeTypeForFile(mediaPath);
        if(mime.name().contains("audio")) {
            thumbnailPath = DEFAULT_ALBUM_ART;
        } else if(mime.name().contains("video")) {
            thumbnailPath = DEFAULT_VIDEO_ART;
        }
    }

    return thumbnailPath;
}

void QThumbnailer::enqueueThumbnailTask(ThumbnailTask* task)
{
    QMimeType mime = s_mimeDatabase.mimeTypeForFile(task->source.path());
    if(mime.name().contains("image")) {
        s_imageQueue.appendTask(task);
    } else {
        s_videoQueue.appendTask(task);
    }
}
