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
#include <QtGui/QImage>
#include <QtGui/QImageReader>
#include <thumbnailer.h>

class QThumbnailer: public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(QThumbnailer)
    Q_ENUMS(Size)

    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(QThumbnailer::Size size READ size WRITE setSize NOTIFY sizeChanged)
    Q_PROPERTY(QUrl thumbnail READ thumbnail NOTIFY thumbnailChanged)

public:
    QThumbnailer(QObject* parent=0);

    enum Size { Small = TN_SIZE_SMALL,
                Large = TN_SIZE_LARGE,
                ExtraLarge = TN_SIZE_XLARGE
              };

    QUrl source() const;
    void setSource(QUrl source);
    QThumbnailer::Size size() const;
    void setSize(QThumbnailer::Size size);
    QUrl thumbnail() const;

Q_SIGNALS:
    void sourceChanged();
    void sizeChanged();
    void thumbnailChanged();

protected:
    void updateThumbnail();
    QString doGetThumbnail(QString mediaPath, QThumbnailer::Size size);
//    ThumbnailSize thumbnailSizeFromQSize(QSize size) const;

private:
    QUrl m_source;
    QThumbnailer::Size m_size;
    QUrl m_thumbnail;
    Thumbnailer m_thumbnailer;
    QImageReader m_reader;
    QMimeDatabase m_mimeDatabase;
};

#endif
