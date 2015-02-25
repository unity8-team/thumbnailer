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

#ifndef THUMBNAILER_H
#define THUMBNAILER_H

#include <QtCore/QObject>
#include <QtCore/QUrl>
#include <QtCore/QMimeDatabase>
#include <QtCore/QPointer>
#include <QtCore/QSize>
#include <QtQml/QQmlParserStatus>
#include <thumbnailer.h>
#include "thumbnailqueue.h"

class ThumbnailTask;

class QThumbnailer : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    Q_DISABLE_COPY(QThumbnailer)

    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(QSize size READ size WRITE setSize NOTIFY sizeChanged)
    Q_PROPERTY(QUrl thumbnail READ thumbnail NOTIFY thumbnailChanged)

    friend class ThumbnailTask;

public:
    QThumbnailer(QObject* parent=0);
    ~QThumbnailer();

    // inherited from QQmlParserStatus
    void classBegin();
    void componentComplete();

    // getters and setters
    QUrl source() const;
    void setSource(QUrl source);
    QSize size() const;
    void setSize(QSize size);
    QUrl thumbnail() const;

Q_SIGNALS:
    void sourceChanged();
    void sizeChanged();
    void thumbnailChanged();

protected:
    void updateThumbnail();
    void cancelUpdateThumbnail();
    static QString thumbnailPathForMedia(QString mediaPath, ThumbnailSize size);
    static void enqueueThumbnailTask(ThumbnailTask* task);

protected Q_SLOTS:
    void setThumbnail(QString thumbnail);

private:
    bool m_componentCompleted;
    QUrl m_source;
    QUrl m_thumbnail;
    QSize m_size;
    ThumbnailSize m_thumbnailsize;
    QPointer<ThumbnailTask> m_currentTask;

    // static class members shared accross all instances of QThumbnailer
    static ThumbnailQueue s_videoQueue;
    static ThumbnailQueue s_imageQueue;
    static Thumbnailer s_thumbnailer;
    static QMimeDatabase s_mimeDatabase;
};

class ThumbnailTask : public QObject, public QRunnable
{
    Q_OBJECT

public:
    virtual void run() {
        try {
            QString thumbnail = QThumbnailer::thumbnailPathForMedia(m_source.path(), m_size);
            Q_EMIT thumbnailPathRetrieved(thumbnail);
        } catch (std::exception &e) {
            // catch all exceptions cause it is not allowed to have unhandled
            // exceptions thrown from a worker thread
        }
        Q_EMIT finished();
    }

    QUrl m_source;
    ThumbnailSize m_size;
    QPointer<QThumbnailer> m_caller;

Q_SIGNALS:
    void thumbnailPathRetrieved(QString thumbnail);
    void finished();
};

#endif
