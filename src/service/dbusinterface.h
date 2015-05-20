/*
 * Copyright (C) 2014 Canonical, Ltd.
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

#include <memory>

#include <QDBusContext>
#include <QDBusUnixFileDescriptor>
#include <QObject>
#include <QSize>
#include <QString>

namespace unity
{

namespace thumbnailer
{

namespace service
{

struct DBusInterfacePrivate;

class DBusInterface : public QObject, protected QDBusContext
{
    Q_OBJECT
public:
    explicit DBusInterface(QObject* parent = nullptr);
    ~DBusInterface();

    DBusInterface(DBusInterface const&) = delete;
    DBusInterface& operator=(DBusInterface&) = delete;

public Q_SLOTS:
    QDBusUnixFileDescriptor GetAlbumArt(QString const& artist, QString const& album, QSize const& requestedSize);
    QDBusUnixFileDescriptor GetArtistArt(QString const& artist, QString const& album, QSize const& requestedSize);
    QDBusUnixFileDescriptor GetThumbnail(QString const& filename,
                                         QDBusUnixFileDescriptor const& filename_fd,
                                         QSize const& requestedSize);

private:
    void queueRequest(Handler* handler);

private Q_SLOTS:
    void requestFinished();

Q_SIGNALS:
    void endInactivity();
    void startInactivity();

private:
    std::unique_ptr<DBusInterfacePrivate> p;
};

}  // namespace service

}  // namespace thumbnailer

}  // namespace unity
