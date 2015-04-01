#!/usr/bin/python3

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

args = GLib.Variant("(sss)", (sys.argv[1], sys.argv[2], "original"))
result, fd_list = proxy.call_with_unix_fd_list_sync(
    "GetAlbumArt", args, Gio.DBusCallFlags.NONE, 500, None, None)

fd = os.fdopen(fd_list.peek_fds()[0], 'rb')
with open(sys.argv[3], 'wb') as output:
    output.write(fd.read())
