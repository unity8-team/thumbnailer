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

#include "dbus_connection.h"

#include <unity/util/DefinesPtrs.h>

#include <string>
#include <vector>

namespace unity
{

namespace thumbnailer
{

namespace tools
{

class Action
{
public:
    UNITY_DEFINES_PTRS(Action);

    virtual ~Action() {}

    virtual void run(DBusConnection& conn) = 0;

protected:
    Action(std::vector<std::string> const& args)
        : args_(args)
    {
    }

    std::vector<std::string> const& args() const noexcept
    {
        return args_;
    }

private:
    std::vector<std::string> args_;
};

}  // namespace tools

}  // namespace thumbnailer

}  // namespace unity
