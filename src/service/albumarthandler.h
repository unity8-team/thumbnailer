/*
 * Copyright (C) 2015 Canonical, Ltd.
 *
 * Authors:
 *    James Henstridge <james.henstridge@canonical.com>
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of version 3 of the GNU General Public License as published
 * by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "handler.h"
#include <thumbnailer.h>

#include <memory>

#include <QObject>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusUnixFileDescriptor>

namespace unity {
namespace thumbnailer {
namespace service {

struct AlbumArtHandlerPrivate;

class AlbumArtHandler : public Handler {
    Q_OBJECT
public:
    AlbumArtHandler(const QDBusConnection &bus, const QDBusMessage &message,
                    const std::shared_ptr<Thumbnailer> &thumbnailer,
                    std::shared_ptr<QThreadPool> check_pool,
                    std::shared_ptr<QThreadPool> create_pool,
                    const QString &artist,
                    const QString &album,
                    const QSize &requestedSize);
    ~AlbumArtHandler();

protected:
    virtual QDBusUnixFileDescriptor check() override;
    virtual void download() override;
    virtual QDBusUnixFileDescriptor create() override;

private:
    std::unique_ptr<AlbumArtHandlerPrivate> p;
};

}
}
}
