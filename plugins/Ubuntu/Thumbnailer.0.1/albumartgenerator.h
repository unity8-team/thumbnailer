/*
 * Copyright 2014 Canonical Ltd.
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
 *          James Henstridge <james.henstridge@canonical.com>
*/

#pragma once

#include <QQuickImageProvider>
#include <thumbnailerinterface.h>

#include <memory>

namespace unity {
namespace thumbnailer {
namespace qml {

class AlbumArtGenerator: public QQuickImageProvider
{
private:
    std::unique_ptr<QDBusConnection> connection;
    std::unique_ptr<ThumbnailerInterface> iface;

public:
    AlbumArtGenerator();
    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize);
};

}
}
}