/*
 * Copyright (C) 2015 Canonical Ltd
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
 * Authored by: Xavi Garcia <xavi.garcia.mena@canonical.com>
 */

#include "inactivityhandler.h"

#include <QCoreApplication>
#include <QDebug>

const int MAX_INACTIVITY_TIME = 5000; // max inactivity time before exiting the app, in milliseconds

using namespace unity::thumbnailer::service;

InactivityHandler::InactivityHandler(DBusInterface & iface) : QObject(&iface) {
    timer_.setSingleShot(true);
    timer_.setInterval(MAX_INACTIVITY_TIME);

    // connect dbus interface inactivity signals to the QTimer start and stop
    connect(&iface, &DBusInterface::endInactivity, &timer_, &QTimer::stop);
    connect(&iface, &DBusInterface::startInactivity, &timer_, static_cast<void (QTimer::*)()>(&QTimer::start));
    connect(&timer_, &QTimer::timeout, QCoreApplication::instance(), &QCoreApplication::quit);
}
