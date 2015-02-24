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

static const QString DEFAULT_VIDEO_ART("/usr/share/thumbnailer/icons/video_missing.png");
static const QString DEFAULT_ALBUM_ART("/usr/share/thumbnailer/icons/album_missing.png");

QThreadPool QThumbnailer::s_videoThreadPool;
QThreadPool QThumbnailer::s_imageThreadPool;
QList<ThumbnailTask*> QThumbnailer::s_videoQueue;
QList<ThumbnailTask*> QThumbnailer::s_imageQueue;
Thumbnailer QThumbnailer::s_thumbnailer;
QMimeDatabase QThumbnailer::s_mimeDatabase;


QThumbnailer::QThumbnailer(QObject* parent) :
    QObject(parent),
    m_componentCompleted(false),
    m_source(""),
    m_thumbnail(""),
    m_size(QThumbnailer::Small),
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

QThumbnailer::Size QThumbnailer::size() const
{
    return m_size;
}

void QThumbnailer::setSize(QThumbnailer::Size size)
{
    if (size != m_size) {
        m_size = size;
        Q_EMIT sizeChanged();
        updateThumbnail();
    }
}

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
    bool slowUpdate = s_thumbnailer.thumbnail_needs_generation(m_source.path().toStdString(),
                                                               (ThumbnailSize)m_size);
    if (!slowUpdate) {
        // if we know retrieving the thumbnail is fast because it is readily
        // available on disk, set it immediately
        QString thumbnail = thumbnailPathForMedia(m_source.path(), m_size);
        setThumbnail(thumbnail);
    } else {
        // otherwise enqueue the thumbnailing task that will then get processed
        // in a background thread and will end up setting the thumbnail URL
        ThumbnailTask* task = new ThumbnailTask;
        task->source = m_source;
        task->size = m_size;
        task->caller = this;
        m_currentTask = task;
        enqueueThumbnailTask(task);
    }
}

void QThumbnailer::cancelUpdateThumbnail()
{
    if (!m_currentTask.isNull()) {
        if (s_imageQueue.removeOne(m_currentTask.data()) ||
            s_videoQueue.removeOne(m_currentTask.data()) ) {
            delete m_currentTask;
            m_currentTask.clear();
        }
    }
}


/* Static methods implementation */

void QThumbnailer::enqueueThumbnailTask(ThumbnailTask* task)
{
    connect(task, SIGNAL(thumbnailPathRetrieved(QString)), task->caller, SLOT(setThumbnail(QString)));

    QMimeType mime = s_mimeDatabase.mimeTypeForFile(task->source.path());
    if(mime.name().contains("image")) {
        connect(task, SIGNAL(thumbnailPathRetrieved(QString)), task->caller, SLOT(processImageQueue()));
        s_imageQueue.append(task);
        QThumbnailer::processImageQueue();
    } else {
        connect(task, SIGNAL(thumbnailPathRetrieved(QString)), task->caller, SLOT(processVideoQueue()));
        s_videoQueue.append(task);
        QThumbnailer::processVideoQueue();
    }
}

QString QThumbnailer::thumbnailPathForMedia(QString mediaPath, QThumbnailer::Size size)
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

void QThumbnailer::processVideoQueue()
{
    if (s_videoQueue.isEmpty()) {
        return;
    }
    ThumbnailTask* task = s_videoQueue.takeFirst();
    if (!s_videoThreadPool.tryStart(task)) {
        s_videoQueue.prepend(task);
    }
}

void QThumbnailer::processImageQueue()
{
    if (s_imageQueue.isEmpty()) {
        return;
    }
    ThumbnailTask* task = s_imageQueue.takeFirst();

    if (!s_imageThreadPool.tryStart(task)) {
        s_imageQueue.prepend(task);
    }
}

