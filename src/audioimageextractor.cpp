/*
 * Copyright (C) 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Jussi Pakkanen <jussi.pakkanen@canonical.com>
 */

#include<cstdio>
#include<internal/audioimageextractor.h>
#include<unistd.h>
#include<stdexcept>
#include<memory>
#include <gst/pbutils/pbutils.h>

using namespace std;

class AudioImageExtractorPrivate {
};

static bool has_tag(const GstTagList *tlist, const gchar *tagname) {
  const GValue *val = gst_tag_list_get_value_index(tlist, tagname, 0);
  return val;
}

static void extract_image(const GstTagList *tlist, FILE *outf) {
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

AudioImageExtractor::AudioImageExtractor() {
    p = new AudioImageExtractorPrivate();
}

AudioImageExtractor::~AudioImageExtractor() {
    delete p;
}


bool AudioImageExtractor::extract(const string &ifname, const string &ofname) {
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
    extract_image(tlist, outfile);
    fclose(outfile);
    return true;
}
