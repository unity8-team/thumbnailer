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
 * Authored by: Michi Henning <michi.henning@canonical.com>
 */

#pragma once

#include <internal/thumbnailer.h>
#include "stats.h"

#include <QDBusContext>
#include <QCoreApplication>

namespace unity
{

namespace thumbnailer
{

namespace service
{

class AdminInterface : public QObject, protected QDBusContext
{
    Q_OBJECT
public:
    AdminInterface(std::shared_ptr<unity::thumbnailer::internal::Thumbnailer> const& thumbnailer,
                   QCoreApplication& app,
                   QObject* parent = nullptr)
        : QObject(parent)
        , thumbnailer_(thumbnailer)
        , app_(app)
    {
    }
    ~AdminInterface() = default;

    AdminInterface(AdminInterface const&) = delete;
    AdminInterface& operator=(AdminInterface&) = delete;

public Q_SLOTS:
    AllStats Stats();
    void Shutdown();

private:
    std::shared_ptr<unity::thumbnailer::internal::Thumbnailer> const& thumbnailer_;
    QCoreApplication& app_;
};

}  // namespace service

}  // namespace thumbnailer

}  // namespace unity
