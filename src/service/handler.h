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

#include <thumbnailer.h>

#include <memory>
#include <string>

#include <QObject>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusUnixFileDescriptor>
#include <QSize>

class QThreadPool;

namespace unity {
namespace thumbnailer {
namespace service {

struct HandlerPrivate;

class Handler : public QObject {
    Q_OBJECT
public:
    Handler(const QDBusConnection &bus, const QDBusMessage &message,
            std::shared_ptr<QThreadPool> check_pool,
            std::shared_ptr<QThreadPool> create_pool,
            std::unique_ptr<ThumbnailRequest> &&request);
    ~Handler();

    Handler(const Handler&) = delete;
    Handler& operator=(Handler&) = delete;

    void begin();

private Q_SLOTS:
    void checkFinished();
    void downloadFinished();
    void createFinished();

Q_SIGNALS:
    void finished();

private:
    void sendThumbnail(const QDBusUnixFileDescriptor &unix_fd);
    void sendError(const QString &error);
    QDBusUnixFileDescriptor check();
    QDBusUnixFileDescriptor create();

    std::unique_ptr<HandlerPrivate> p;
};

}
}
}
