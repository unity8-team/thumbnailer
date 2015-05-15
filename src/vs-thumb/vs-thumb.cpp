/*
 * Copyright (C) 2013 Canonical Ltd.
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
 * Authored by: Jussi Pakkanen <jussi.pakkanen@canonical.com>
 */

#include <cstdio>
#include <string>
#include <memory>
#include <stdexcept>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wcast-align"
#include <gst/gst.h>
#include <gio/gio.h>
#include <glib.h>
#pragma GCC diagnostic pop

#include "thumbnailextractor.h"

using namespace std;
using namespace unity::thumbnailer::internal;

namespace
{

string command_line_arg_to_uri(string const& arg)
{
    unique_ptr<GFile, decltype(&g_object_unref)> file(g_file_new_for_commandline_arg(arg.c_str()), g_object_unref);
    if (!file)
    {
        throw runtime_error("Could not create parse argument as file");
    }
    char* c_uri = g_file_get_uri(file.get());
    if (!c_uri)
    {
        throw runtime_error("Could not convert to uri");
    }
    string uri(c_uri);
    g_free(c_uri);
    return uri;
}

bool extract_thumbnail(string const& uri, string const& ofname)
{
    ThumbnailExtractor extractor;

    extractor.set_uri(uri);
    if (extractor.has_video())
    {
        if (!extractor.extract_video_frame())
        {
            return false;
        }
    }
    else
    {
        if (!extractor.extract_audio_cover_art())
        {
            return false;
        }
    }
    extractor.save_screenshot(ofname);
    return true;
}
}

int main(int argc, char** argv)
{
    gst_init(&argc, &argv);
    if (argc != 3)
    {
        fprintf(stderr, "%s <source file> <output file>\n", argv[0]);
        return 1;
    }
    string uri;
    string outfile(argv[2]);
    bool success = false;

    try
    {
        uri = command_line_arg_to_uri(argv[1]);
    }
    catch (exception const& e)
    {
        fprintf(stderr, "Error parsing \"%s\": %s\n", argv[1], e.what());
        return 1;
    }

    try
    {
        success = extract_thumbnail(uri, outfile);
    }
    catch (runtime_error const& e)
    {
        fprintf(stderr, "Error creating thumbnail: %s\n", e.what());
        return 2;
    }

    return success ? 0 : 1;
}
