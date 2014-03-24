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

#ifndef DBUSINTERFACE_H
#define DBUSINTERFACE_H

#include <string>

struct DBusInterfacePrivate;
typedef struct _GDBusConnection GDBusConnection;

class DBusInterface final {
public:
    DBusInterface(GDBusConnection *bus, const std::string& bus_path);
    ~DBusInterface();

    DBusInterface(const DBusInterface&) = delete;
    DBusInterface& operator=(DBusInterface&) = delete;

private:
    DBusInterfacePrivate *p;
};

#endif
