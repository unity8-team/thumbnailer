#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# Copyright (C) 2014 Canonical Ltd
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

import base64
import json
import os
import tornado.httpserver
import tornado.ioloop
import tornado.netutil
import tornado.web
import sys
import tornado.options
from httplib2 import RETRIES

tornado.options.parse_command_line()

global PORT
PORT = ""

def read_file(path, replace_root):
    file = os.path.join(os.path.dirname(__file__), path)
    if os.path.isfile(file):
        mode = 'r'
        if (not replace_root):
            mode = 'rb'
        with open(file, mode) as fp:
            content = fp.read()
        if(replace_root):
            content = content.replace("DOWNLOAD_ROOT", "127.0.0.1:%s" % PORT )
    else:
        raise Exception("File '%s' not found\n" % file)
    return content

class ErrorHandler(tornado.web.RequestHandler):
    def write_error(self, status_code, **kwargs):
        self.write(json.dumps({'error': '%s: %d' % (kwargs["exc_info"][1], status_code)}))

class UbuntuArtistImagesProvider(ErrorHandler):
    def get(self):
        if self.get_argument('artist', None) == "test_threads":
            self.write("TEST_THREADS_TEST_%s" % self.get_argument('album', None ))
        else:
            file = 'images/%s_%s.png' % (self.get_argument('artist', None ), self.get_argument('album', None ))
            self.write(read_file(file, False))
        self.finish()

class UbuntuAlbumImagesProvider(ErrorHandler):
    def get(self):

        if self.get_argument('artist', None) == "test_threads":
            self.write("TEST_THREADS_TEST_%s" % self.get_argument('album', None ))
        else:
            file = 'images/%s_%s.png' % (self.get_argument('artist', None ), self.get_argument('album', None ))
            self.write(read_file(file, False))
        self.finish()

class LastFMArtistAlbumInfo(ErrorHandler):
    def get(self, artist, album):
        file = 'queries/%s_%s.xml' % (artist, album)
        self.write(read_file(file, True))
        self.finish()

class LastFMImagesProvider(ErrorHandler):
    def get(self, image):
        if(image.startswith("test_thread")):
            self.write("TEST_THREADS_TEST_%s" % image)
        else:
            file = 'images/%s.png' % image
            self.write(read_file(file, False))
        self.finish()

def validate_argument(self, name, expected):
    actual = self.get_argument(name, '')
    if actual != expected:
        raise Exception("Argument '%s' == '%s' != '%s'" % (name, actual, expected))

def validate_header(self, name, expected):
    actual = self.request.headers.get(name, '')
    if actual != expected:
        raise Exception("Header '%s' == '%s' != '%s'" % (name, actual, expected))

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
