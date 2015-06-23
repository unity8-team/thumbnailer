/*
 * Copyright (C) 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Michi Henning <michi@canonical.com>
 */

#pragma once

#include <QDebug>

namespace unity
{

namespace thumbnailer
{

namespace internal
{

class TraceMessageHandlerInitializer final
{
public:
    TraceMessageHandlerInitializer();
    ~TraceMessageHandlerInitializer();
};

static TraceMessageHandlerInitializer trace_message_handler_initializer_;  // Schwartz counter

}  // namespace internal

}  // namespace thumbnailer

}  // namespace unity