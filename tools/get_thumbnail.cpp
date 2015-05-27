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

#include "get_thumbnail.h"

#include <cassert>
#include <iomanip>  // TODO: needed?
#include <iostream>  // TODO: needed?

#include <inttypes.h>  // TODO: needed?
#include <stdio.h>  // TODO: needed?

using namespace std;

namespace unity
{

namespace thumbnailer
{

namespace tools
{

GetThumbnail::GetThumbnail(QStringList const& args)
    : Action(args)
{
    assert(args[1] == QString("get") ||
           args[1] == QString("artist") ||
           args[1] == QString("album"));
    // TODO: parsing
    if (args.size() == 2)
    {
        return;
    }
    if (args.size() > 4)
    {
        throw "too many arguments for stats command";
    }
}

GetThumbnail::~GetThumbnail() {}

namespace
{

}

void GetThumbnail::run(DBusConnection& conn)
{
    qDBusRegisterMetaType<unity::thumbnailer::service::AllStats>();

//const QString &filename, const QDBusUnixFileDescriptor &filename_fd, const QSize &requestedSize
#if 0
    auto reply = conn.thumbnailer().GetThumbnail();
    reply.waitForFinished();
    if (!reply.isValid())
    {
        throw reply.error().message();
    }
    auto st = reply.value();
#endif
}

}  // namespace tools

}  // namespace thumbnailer

}  // namespace unity
