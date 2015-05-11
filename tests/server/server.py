#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# Copyright (C) 2015 Canonical Ltd
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License version 3 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# Authored by: Xavi Garcia <xavi.garcia.mena@canonical.com>

import os
import tornado.httpserver
import tornado.web
import sys
import tornado.options

tornado.options.parse_command_line()

global PORT
PORT = ""

class FileReaderProvider(tornado.web.RequestHandler):
    def initialize(self):
        self.extensions_map = {'jpeg': 'image/jpeg', 'jpg': 'image/jpeg', 'png': 'image/png', 'txt': 'text/plain', 'xml': 'application/xml'}

    def read_file(self, path, replace_root):
        extension = self.search_file_extension(path)
        if extension != None:
            file = os.path.join(os.path.dirname(__file__), "%s.%s" % (path, extension))
            with open(file, 'rb') as fp:
                content = fp.read()
            if replace_root:
                content = content.replace(b"DOWNLOAD_ROOT", "127.0.0.1:{}".format(PORT).encode("utf-8"))
            self.set_header("Content-Type", self.extensions_map[extension])
        else:
            self.set_status(404)
            content = "<html><body>404 ERROR</body></html>"
        self.write(content)

    def search_file_extension(self, file_base_name):
        for extension in self.extensions_map.keys():
            file = os.path.join(os.path.dirname(__file__), "%s.%s" % (file_base_name, extension))
            if os.path.isfile(file):
                return extension
        return None

class UbuntuAlbumImagesProvider(FileReaderProvider):
    def get(self):
        if self.get_argument('artist', None) == "test_threads":
            self.write("TEST_THREADS_TEST_%s" % self.get_argument('album', None ))
        else:
            file = 'images/%s_%s_album' % (self.get_argument('artist', None ), self.get_argument('album', None ))
            self.read_file(file, False)
        self.finish()

class UbuntuArtistImagesProvider(FileReaderProvider):
    def get(self):
        if self.get_argument('artist', None) == "test_threads":
            self.write("TEST_THREADS_TEST_%s" % self.get_argument('album', None ))
        else:
            file = 'images/%s_%s' % (self.get_argument('artist', None ), self.get_argument('album', None ))
            self.read_file(file, False)
        self.finish()

def new_app():
    application = tornado.web.Application([
        (r"/musicproxy/v1/album-art", UbuntuAlbumImagesProvider),
        (r"/musicproxy/v1/artist-art", UbuntuArtistImagesProvider)
    ], gzip=True)
    sockets = tornado.netutil.bind_sockets(0, '127.0.0.1')
    server = tornado.httpserver.HTTPServer(application)
    server.add_sockets(sockets)

    global PORT
    PORT = '%d' % sockets[0].getsockname()[1]
    sys.stdout.write('%s\n' % PORT)
    sys.stdout.flush()

    return application

if __name__ == "__main__":
    application = new_app()
    tornado.ioloop.IOLoop.instance().start()
