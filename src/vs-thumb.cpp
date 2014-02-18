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
#include<memory>
#include<stdexcept>
#include<gst/gst.h>
#include<gst/app/gstappsink.h>
#include<gdk-pixbuf/gdk-pixbuf.h>

using namespace std;

bool extract(const std::string &ifname, const std::string &ofname) {
    string caps_string = "video/x-raw,format=RGB,pixel-aspect-ratio=1/1";
    GstElement *sink;
    gint64 duration;
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
    }
    gst_element_query_duration(pipeline.get(), GST_FORMAT_TIME, &duration);
    gst_element_seek_simple(pipeline.get(),
                            GST_FORMAT_TIME,
                            static_cast<GstSeekFlags>(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT),
                            2*duration/7);
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

int main(int argc, char **argv) {
    gst_init(&argc, &argv);
    if(argc != 3) {
        printf("%s <source file> <output file>\n", argv[0]);
        return 1;
    }
    string infile(argv[1]);
    string outfile(argv[2]);
    bool success;
    try {
        success = extract(infile, outfile);
    } catch(runtime_error &e) {
        return 2;
    }
    if(success) {
        return 0;
    }
    return 1;
}
