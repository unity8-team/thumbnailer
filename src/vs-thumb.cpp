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

using namespace std;

namespace {

bool extract_video(const std::string &ifname, const std::string &ofname) {
    string caps_string = "video/x-raw,format=RGB,pixel-aspect-ratio=1/1";
    GstElement *sink;
    gint64 duration, seek_point;
    string pipe_cmd = "filesrc location=\"";
    pipe_cmd += ifname;
    pipe_cmd += "\" ! decodebin ! videoconvert ! videoscale !";
    pipe_cmd += "appsink name=sink caps=\"";
    pipe_cmd += caps_string;
    pipe_cmd += "\"";

    unique_ptr<GstElement, void(*)(GstElement *t)> pipeline(
            gst_parse_launch(pipe_cmd.c_str(), nullptr),
            [](GstElement *t){ gst_object_unref(t);});
    sink = gst_bin_get_by_name(GST_BIN(pipeline.get()), "sink");

    GstStateChangeReturn ret = gst_element_set_state(pipeline.get(), GST_STATE_PAUSED);
    switch (ret) {
        case GST_STATE_CHANGE_FAILURE:
            throw runtime_error("Fail to start thumbnail pipeline.");
        case GST_STATE_CHANGE_NO_PREROLL:
            throw runtime_error("Thumbnail not supported for live sources.");
        default:
            break;
    }
    // Need to preroll in order to get duration.
    ret = gst_element_get_state(pipeline.get(), nullptr, nullptr, 3*GST_SECOND);
    if(ret == GST_STATE_CHANGE_FAILURE) {
      throw runtime_error("Failed to preroll.");
    } else if(ret == GST_STATE_CHANGE_ASYNC) {
        fprintf(stderr, "Problem prerolling, trying to continue anyway.\n");
    }
    if(!gst_element_query_duration(pipeline.get(), GST_FORMAT_TIME, &duration)) {
        fprintf(stderr, "Media backend does not implement query_duration, using fallback time.\n");
        seek_point = 10*GST_SECOND;
    } else {
        seek_point = 2*duration/7;
    }
    gst_element_seek_simple(pipeline.get(),
                            GST_FORMAT_TIME,
                            static_cast<GstSeekFlags>(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT),
                            seek_point);
    GstSample *sample = nullptr;
    sample = gst_app_sink_pull_preroll(GST_APP_SINK(sink));
    if(sample == nullptr || !GST_IS_SAMPLE(sample)) {
        throw runtime_error("Could not create sample. Unsupported or invalid file type?");
    }
    GstCaps *caps = gst_sample_get_caps(sample);

    GstBuffer *buf = gst_sample_get_buffer(sample);
    if (sample) {
        gint width, height;
        GstStructure *s = gst_caps_get_structure(caps, 0);
        gboolean res = gst_structure_get_int(s, "width", &width);
        res |= gst_structure_get_int (s, "height", &height);
        if (!res) {
            throw runtime_error("could not determine snapshot dimension.");
        }
        GstMapInfo mi;
        gst_buffer_map(buf, &mi, GST_MAP_READ);
        unique_gobj<GdkPixbuf> img(
                gdk_pixbuf_new_from_data(mi.data, GDK_COLORSPACE_RGB,
                        false, 8, width, height,
                        (((width * 3)+3)&~3),
                        nullptr, nullptr));
        gst_buffer_unmap(buf, &mi);
        GError *err = nullptr;
        if(!gdk_pixbuf_save(img.get(), ofname.c_str(), "jpeg", &err, NULL)) {
            string msg = err->message;
            g_error_free(err);
            throw runtime_error(msg);
        }
    } else {
      throw runtime_error("Pipeline failed.");
    }

    gst_element_set_state(pipeline.get(), GST_STATE_NULL);
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

bool extract_audio(const std::string &ifname, const std::string &ofname) {
    string uri("file://");
    if(ifname[0] != '/') {
        uri += getcwd(nullptr, 0);
        uri += '/';
    }
    uri += ifname;
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
            g_file_new_for_path(infile.c_str()), g_object_unref);
    if(!file) {
        return 1;
    }

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
            success = extract_audio(infile, outfile);
        } catch(runtime_error &e) {
            printf("Error creating thumbnail: %s\n", e.what());
            return 2;
        }
    }

    if (content_type.find("video/") == 0) {
        try {
            success = extract_video(infile, outfile);
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
