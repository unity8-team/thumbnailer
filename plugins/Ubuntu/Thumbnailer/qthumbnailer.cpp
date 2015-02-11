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

QThumbnailer::QThumbnailer(QObject* parent) :
    QObject(parent),
    m_size(QThumbnailer::Large)
{
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

void QThumbnailer::updateThumbnail()
{
    QString thumbnailPath = doGetThumbnail(m_source.path(), m_size);
    m_thumbnail = QUrl::fromLocalFile(thumbnailPath);
    Q_EMIT thumbnailChanged();
}

QString QThumbnailer::doGetThumbnail(QString mediaPath, QThumbnailer::Size size)
{
    QString thumbnailPath;

    thumbnailPath = QString::fromStdString(m_thumbnailer.get_thumbnail(mediaPath.toStdString(),
                                           (ThumbnailSize)size));

    if(thumbnailPath.isEmpty()) {
        QMimeType mime = m_mimeDatabase.mimeTypeForFile(mediaPath);
        if(mime.name().contains("audio")) {
            thumbnailPath = DEFAULT_ALBUM_ART;
        } else if(mime.name().contains("video")) {
            thumbnailPath = DEFAULT_VIDEO_ART;
        }
    }

    return thumbnailPath;
}


//ThumbnailSize QThumbnailer::thumbnailSizeFromQSize(QSize size) const
//{
//    ThumbnailSize thumbnailSize;
//    const int xlarge_cutoff = 512;
//    const int large_cutoff = 256;
//    const int small_cutoff = 128;
//    if(size.width() > xlarge_cutoff || size.height() > xlarge_cutoff) {
//        thumbnailSize = TN_SIZE_ORIGINAL;
//    } else if(size.width() > large_cutoff || size.height() > large_cutoff) {
//        thumbnailSize = TN_SIZE_XLARGE;
//    } else if(size.width() > small_cutoff || size.height() > small_cutoff) {
//        thumbnailSize = TN_SIZE_LARGE;
//    } else {
//        thumbnailSize = TN_SIZE_SMALL;
//    }
//    return thumbnailSize;
//}
