#!/usr/bin/python3

#
# Copyright (C) 2014 Canonical Ltd
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 3 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# Authored by: James Henstridge <james.henstridge@canonical.com>
#

import os
import sys

from gi.repository import GLib, Gio

if len(sys.argv) < 3:
    sys.stderr.write("usage: get-thumbnail.py FILENAME OUTPUT.png\n")
    sys.exit(1)

bus = Gio.bus_get_sync(Gio.BusType.SESSION, None)
proxy = Gio.DBusProxy.new_sync(bus, Gio.DBusProxyFlags.NONE, None,
                               "com.canonical.Thumbnailer",
                               "/com/canonical/Thumbnailer",
                               "com.canonical.Thumbnailer", None)

filename = Gio.File.new_for_commandline_arg(sys.argv[1]).get_path()

args = GLib.Variant("(sh(ii))", (filename, 0, (-1, -1)))
args_fds = Gio.UnixFDList.new()
args_fds.append(os.open(filename, os.O_RDONLY))

result, fd_list = proxy.call_with_unix_fd_list_sync(
    "GetThumbnail", args, Gio.DBusCallFlags.NONE, 5000, args_fds, None)

fd = os.fdopen(fd_list.peek_fds()[0], 'rb')
with open(sys.argv[2], 'wb') as output:
    output.write(fd.read())
