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

#include<internal/gobj_memory.h>
#include<cstdio>
#include<string>
#include<memory>
#include<stdexcept>
#include<gst/gst.h>
#include<gst/app/gstappsink.h>
#include<gdk-pixbuf/gdk-pixbuf.h>
#include<gio/gio.h>
#include<glib.h>
#include<gst/pbutils/pbutils.h>

#include "thumbnailextractor.h"

using namespace std;

namespace {

bool extract_video(const std::string &uri, const std::string &ofname) {
    ThumbnailExtractor extractor;

    extractor.set_uri(uri);
    extractor.extract_frame();
    extractor.save_screenshot(ofname);
    return true;
}

bool has_tag(const GstTagList *tlist, const gchar *tagname) {
  const GValue *val = gst_tag_list_get_value_index(tlist, tagname, 0);
  return val;
}

void extract_embedded_image(const GstTagList *tlist, FILE *outf) {
  const GValue *val = gst_tag_list_get_value_index(tlist, GST_TAG_IMAGE, 0);
  GstSample *sample = (GstSample*)g_value_get_boxed(val);
  GstBuffer *buf = gst_sample_get_buffer(sample);

  for(guint i=0; i<gst_buffer_n_memory(buf); i++) {
    GstMemory *mem = gst_buffer_peek_memory(buf, i);
    GstMapInfo mi;
    gst_memory_map(mem, &mi, GST_MAP_READ);
    fwrite(mi.data, 1, mi.size, outf);
    gst_memory_unmap(mem, &mi);
  }
}

bool extract_audio(const std::string &uri, const std::string &ofname) {
    GError *err = nullptr;
    unique_ptr<GstDiscoverer, void(*)(GstDiscoverer*)> dsc(
            gst_discoverer_new(GST_SECOND, &err),
            [](GstDiscoverer *t) {g_object_unref(G_OBJECT(t));});
    if(err) {
        string msg(err->message);
        g_error_free(err);
        throw runtime_error(msg);
    }
    unique_ptr<GstDiscovererInfo, void(*)(GstDiscovererInfo*)> info(
            gst_discoverer_discover_uri(dsc.get(), uri.c_str(), &err),
            [](GstDiscovererInfo *t) {g_object_unref(G_OBJECT(t));});
    if(err) {
        string msg(err->message);
        g_error_free(err);
        throw runtime_error(msg);
    }
    const GstTagList *tlist = gst_discoverer_info_get_tags(info.get());
    if(!tlist) {
        return false;
    }
    if(!has_tag(tlist, GST_TAG_IMAGE)) {
        return false;
    }
    FILE *outfile = fopen(ofname.c_str(), "wb");
    extract_embedded_image(tlist, outfile);
    fclose(outfile);
    return true;
}

}

int main(int argc, char **argv) {
    gst_init(&argc, &argv);
    if(argc != 3) {
        printf("%s <source file> <output file>\n", argv[0]);
        return 1;
    }
    string infile(argv[1]);
    string outfile(argv[2]);
    bool success = false;

    // Determine file type.
    std::unique_ptr<GFile, void(*)(void *)> file(
            g_file_new_for_commandline_arg(infile.c_str()), g_object_unref);
    if(!file) {
        return 1;
    }

    std::unique_ptr<char, void(*)(void *)> c_uri(
        g_file_get_uri(file.get()), g_free);
    std::string uri(c_uri.get());

    std::unique_ptr<GFileInfo, void(*)(void *)> info(
            g_file_query_info(file.get(), G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
                G_FILE_QUERY_INFO_NONE, /* cancellable */ NULL, /* error */NULL),
            g_object_unref);
    if(!info) {
        return 1;
    }

    std::string content_type(g_file_info_get_content_type(info.get()));

    if (content_type.find("audio/") == 0) {
        try {
            success = extract_audio(uri, outfile);
        } catch(runtime_error &e) {
            printf("Error creating thumbnail: %s\n", e.what());
            return 2;
        }
    }

    if (content_type.find("video/") == 0) {
        try {
            success = extract_video(uri, outfile);
        } catch(runtime_error &e) {
            printf("Error creating thumbnail: %s\n", e.what());
            return 2;
        }
    }

    if(success) {
        return 0;
    }
    return 1;
}
