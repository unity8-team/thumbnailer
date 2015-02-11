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

QThreadPool QThumbnailer::c_videoThreadPool;
QThreadPool QThumbnailer::c_imageThreadPool;
Thumbnailer QThumbnailer::c_thumbnailer;
QMimeDatabase QThumbnailer::c_mimeDatabase;
QList<ThumbnailTask*> QThumbnailer::c_videoQueue;
QList<ThumbnailTask*> QThumbnailer::c_imageQueue;

QThumbnailer::QThumbnailer(QObject* parent) :
    QObject(parent),
    m_completed(false),
    m_size(QThumbnailer::Small)
{
}

QThumbnailer::~QThumbnailer()
{
    cancelCurrentTask();
}

void QThumbnailer::classBegin()
{
}

void QThumbnailer::componentComplete()
{
    m_completed = true;
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
    if (!m_completed) {
        return;
    }

    cancelCurrentTask();
    bool slowUpdate = c_thumbnailer.thumbnail_needs_generation(m_source.path().toStdString(),
                                                               (ThumbnailSize)m_size);
    if (slowUpdate) {
        ThumbnailTask* task = new ThumbnailTask;
        task->source = m_source;
        task->size = m_size;
        enqueueThumbnailTask(task);
    } else {
        QString thumbnail = doGetThumbnail(m_source.path(), m_size);
        setThumbnail(thumbnail);
    }
}

void QThumbnailer::enqueueThumbnailTask(ThumbnailTask* task)
{
    connect(task, SIGNAL(finished(QString)), this, SLOT(setThumbnail(QString)));

    QMimeType mime = c_mimeDatabase.mimeTypeForFile(task->source.path());
    if(mime.name().contains("image")) {
        connect(task, SIGNAL(finished(QString)), this, SLOT(processImageQueue()));
        c_imageQueue.append(task);
        processImageQueue();
    } else {
        connect(task, SIGNAL(finished(QString)), this, SLOT(processVideoQueue()));
        c_videoQueue.append(task);
        processVideoQueue();
    }
}

void QThumbnailer::processVideoQueue()
{
    if (c_videoQueue.isEmpty()) {
        return;
    }
    ThumbnailTask* task = c_videoQueue.takeFirst();
    if (!c_videoThreadPool.tryStart(task)) {
        c_videoQueue.prepend(task);
    }
}

void QThumbnailer::processImageQueue()
{
    if (c_imageQueue.isEmpty()) {
        return;
    }
    ThumbnailTask* task = c_imageQueue.takeFirst();

    if (!c_imageThreadPool.tryStart(task)) {
        c_imageQueue.prepend(task);
    }
}

QString QThumbnailer::doGetThumbnail(QString mediaPath, QThumbnailer::Size size)
{
    QString thumbnailPath;
    try {
        thumbnailPath = QString::fromStdString(c_thumbnailer.get_thumbnail(mediaPath.toStdString(),
                                               (ThumbnailSize)size));
    } catch(std::runtime_error &e) {
        qWarning() << QString("Thumbnail retrieval for %1 failed:").arg(mediaPath).toLocal8Bit().constData() << e.what();
    }

    if(thumbnailPath.isEmpty()) {
        QMimeType mime = c_mimeDatabase.mimeTypeForFile(mediaPath);
        if(mime.name().contains("audio")) {
            thumbnailPath = DEFAULT_ALBUM_ART;
        } else if(mime.name().contains("video")) {
            thumbnailPath = DEFAULT_VIDEO_ART;
        }
    }

    return thumbnailPath;
}

void QThumbnailer::cancelCurrentTask()
{
    if (!m_currentTask.isNull()) {
        if (c_imageQueue.removeOne(m_currentTask.data()) ||
            c_videoQueue.removeOne(m_currentTask.data()) ) {
            QSharedPointer<ThumbnailTask> previousTask = m_currentTask.toStrongRef();
            m_currentTask.clear();
            previousTask.clear();
        }
    }
}


void ThumbnailTask::run()
{
    QString result = QThumbnailer::doGetThumbnail(source.path(), size);
    Q_EMIT finished(result);
}
