#!/usr/bin/python3

#
# Copyright (C) 2015 Canonical Ltd
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

if len(sys.argv) < 4:
    sys.stderr.write("usage: get-album-art.py ARTIST ALBUM OUTPUT.jpg\n")
    sys.exit(1)

bus = Gio.bus_get_sync(Gio.BusType.SESSION, None)
proxy = Gio.DBusProxy.new_sync(bus, Gio.DBusProxyFlags.NONE, None,
                               "com.canonical.Thumbnailer",
                               "/com/canonical/Thumbnailer",
                               "com.canonical.Thumbnailer", None)

args = GLib.Variant("(ss(ii))", (sys.argv[1], sys.argv[2], (-1, -1)))
result, fd_list = proxy.call_with_unix_fd_list_sync(
    "GetAlbumArt", args, Gio.DBusCallFlags.NONE, 5000, None, None)

fd = os.fdopen(fd_list.peek_fds()[0], 'rb')
with open(sys.argv[3], 'wb') as output:
    output.write(fd.read())
