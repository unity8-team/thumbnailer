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

#pragma once

#include <memory>

#include <QQuickImageProvider>
#include <thumbnailerinterface.h>

namespace unity {
namespace thumbnailer {
namespace qml {

class ThumbnailGenerator: public QQuickImageProvider
{
private:
    std::unique_ptr<QDBusConnection> connection;
    std::unique_ptr<ThumbnailerInterface> iface;

public:
    ThumbnailGenerator();
    ThumbnailGenerator(const ThumbnailGenerator &other) = delete;
    const ThumbnailGenerator & operator=(const ThumbnailGenerator &other) = delete;
    ThumbnailGenerator(ThumbnailGenerator &&other) = delete;
    const ThumbnailGenerator & operator=(ThumbnailGenerator &&other) = delete;

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;
    QImage getFallbackImage(const QString &id, QSize *size, const QSize &requestedSize);
};

}
}
}
