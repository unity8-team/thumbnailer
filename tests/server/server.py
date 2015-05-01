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

import imghdr
import os
import tornado.httpserver
import tornado.web
import sys
import tornado.options

tornado.options.parse_command_line()

global PORT
PORT = ""

class FileReaderProvider(tornado.web.RequestHandler):
    def read_file(self, path, replace_root):
        image_type = None
        file = os.path.join(os.path.dirname(__file__), path)
        if os.path.isfile(file):
            with open(file, 'rb') as fp:
                content = fp.read()
            if replace_root:
                content = content.replace(b"DOWNLOAD_ROOT", "127.0.0.1:{}".format(PORT).encode("utf-8"))
            # determine if the content is a real image
            image_type = imghdr.what(file)
        else:
            self.set_status(404)
            content = "<html><body>404 ERROR</body></html>"
        if (image_type != None):
            self.set_header("Content-Type", 'image/%s' % image_type)
        self.write(content)

class UbuntuAlbumImagesProvider(FileReaderProvider):
    def get(self):
        if self.get_argument('artist', None) == "test_threads":
            self.write("TEST_THREADS_TEST_%s" % self.get_argument('album', None ))
        else:
            file = 'images/%s_%s_album.png' % (self.get_argument('artist', None ), self.get_argument('album', None ))
            self.read_file(file, False)
        self.finish()

class UbuntuArtistImagesProvider(FileReaderProvider):
    def get(self):
        if self.get_argument('artist', None) == "test_threads":
            self.write("TEST_THREADS_TEST_%s" % self.get_argument('album', None ))
        else:
            file = 'images/%s_%s.png' % (self.get_argument('artist', None ), self.get_argument('album', None ))
            self.read_file(file, False)
        self.finish()

class LastFMArtistAlbumInfo(FileReaderProvider):
    def get(self, artist, album):
        file = 'queries/%s_%s.xml' % (artist, album)
        self.read_file(file, True)
        self.finish()

class LastFMImagesProvider(FileReaderProvider):
    def get(self, image):
        if(image.startswith("test_thread")):
            self.write("TEST_THREADS_TEST_%s" % image)
        else:
            file = 'images/%s.png' % image
            self.read_file(file, False)
        self.finish()

def new_app():
    application = tornado.web.Application([
        (r"/1.0/album/(\w+)/(\w+)/info.xml", LastFMArtistAlbumInfo),
        (r"/images/(\w+).png", LastFMImagesProvider),
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
