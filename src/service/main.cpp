/*
 * Copyright (C) 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: James Henstridge <james.henstridge@canonical.com>
 */

#include <cstdio>
#include <iostream>

#include <QCoreApplication>

#include "dbusinterface.h"
#include "dbusinterfaceadaptor.h"
#include "inactivityhandler.h"

using namespace unity::thumbnailer::service;

static const char BUS_NAME[] = "com.canonical.Thumbnailer";
static const char BUS_PATH[] = "/com/canonical/Thumbnailer";

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);

    auto bus = QDBusConnection::sessionBus();

    unity::thumbnailer::service::DBusInterface server;
    new ThumbnailerAdaptor(&server);
    bus.registerObject(BUS_PATH, &server);

    if (!bus.registerService(BUS_NAME))
    {
        fprintf(stderr, "Could no acquire D-Bus name %s.\n", BUS_NAME);
        return 0;
    }
    try
    {
        new InactivityHandler(server);
    }
    catch (std::invalid_argument& e)
    {
        std::cerr << e.what() << std::endl;
        exit(1);
    }
    return app.exec();
}
