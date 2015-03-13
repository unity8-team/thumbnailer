#include "thumbnailextractor.h"

#include <cstdio>
#include <stdexcept>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gst/gst.h>

#include <internal/gobj_memory.h>

struct ThumbnailExtractor::Private {
    unique_gobj<GstElement> playbin;
    gint64 duration = -1;

    Private();
    void change_state(GstState state);
};

ThumbnailExtractor::ThumbnailExtractor()
    : p(new Private) {
}

ThumbnailExtractor::~ThumbnailExtractor() {
    p->change_state(GST_STATE_NULL);
}

/* GstPlayFlags flags from playbin */
typedef enum {
  GST_PLAY_FLAG_VIDEO         = (1 << 0),
  GST_PLAY_FLAG_AUDIO         = (1 << 1),
  GST_PLAY_FLAG_TEXT          = (1 << 2),
  GST_PLAY_FLAG_VIS           = (1 << 3),
  GST_PLAY_FLAG_SOFT_VOLUME   = (1 << 4),
  GST_PLAY_FLAG_NATIVE_AUDIO  = (1 << 5),
  GST_PLAY_FLAG_NATIVE_VIDEO  = (1 << 6),
  GST_PLAY_FLAG_DOWNLOAD      = (1 << 7),
  GST_PLAY_FLAG_BUFFERING     = (1 << 8),
  GST_PLAY_FLAG_DEINTERLACE   = (1 << 9),
  GST_PLAY_FLAG_SOFT_COLORBALANCE = (1 << 10),
  GST_PLAY_FLAG_FORCE_FILTERS = (1 << 11),
} GstPlayFlags;

ThumbnailExtractor::Private::Private() {
    GstElement *pb = gst_element_factory_make("playbin", "playbin");
    if (!pb) {
        throw std::runtime_error("Could not create playbin");
    }
    playbin.reset(static_cast<GstElement*>(g_object_ref_sink(pb)));

    GstElement *audio_sink = gst_element_factory_make("fakesink", "audio-fake-sink");
    if (!audio_sink) {
        throw std::runtime_error("Could not create audio sink");
    }
    GstElement *video_sink = gst_element_factory_make("fakesink", "video-fake-sink");
    if (!video_sink) {
        g_object_unref(audio_sink);
        throw std::runtime_error("Could not create video sink");
    }

    g_object_set(video_sink, "sync", TRUE, nullptr);
    g_object_set(playbin.get(),
                 "audio-sink", audio_sink,
                 "video-sink", video_sink,
                 "flags", GST_PLAY_FLAG_VIDEO,
                 nullptr);
}

void ThumbnailExtractor::Private::change_state(GstState state) {
    GstStateChangeReturn ret = gst_element_set_state(playbin.get(), state);
    switch (ret) {
    case GST_STATE_CHANGE_NO_PREROLL:
    case GST_STATE_CHANGE_SUCCESS:
        return;
    case GST_STATE_CHANGE_ASYNC:
        // The change is happening in a background thread, which we
        // will wait on below.
        break;
    case GST_STATE_CHANGE_FAILURE:
    default:
        throw std::runtime_error("Could not change element state");
    }

    // We're in the async case here, so pop messages off the bus until
    // it is done.
    unique_gobj<GstBus> bus(gst_element_get_bus(playbin.get()));
    while (true) {
        std::unique_ptr<GstMessage, decltype(&gst_message_unref)> message(
            gst_bus_timed_pop_filtered(
                bus.get(), GST_CLOCK_TIME_NONE,
                static_cast<GstMessageType>(GST_MESSAGE_ASYNC_DONE | GST_MESSAGE_ERROR)),
            gst_message_unref);
        if (!message) {
            break;
        }
        
        switch (GST_MESSAGE_TYPE(message.get())) {
        case GST_MESSAGE_ASYNC_DONE:
            if (GST_MESSAGE_SRC(message.get()) == GST_OBJECT(playbin.get())) {
                return;
            }
            break;
        case GST_MESSAGE_ERROR:
        {
            GError *error = nullptr;
            gst_message_parse_error(message.get(), &error, nullptr);
            std::string errormsg(error->message);
            g_error_free(error);
            throw std::runtime_error(
                std::string("Error changing element state: ") + errormsg);
            break;
        }
        default:
            /* ignore other message types */
            ;
        }
    }
}

void ThumbnailExtractor::set_uri(const std::string &uri) {
    fprintf(stderr, "Changing to state NULL\n");
    p->change_state(GST_STATE_NULL);
    g_object_set(p->playbin.get(), "uri", uri.c_str(), nullptr);
    fprintf(stderr, "Changing to state PAUSED\n");
    p->change_state(GST_STATE_PAUSED);

    if (!gst_element_query_duration (p->playbin.get(), GST_FORMAT_TIME, &p->duration)) {
        p->duration = -1;
    }
}

void ThumbnailExtractor::seek_sample_frame() {
    gint64 seek_point = 10 * GST_SECOND;
    if (p->duration >= 0) {
        seek_point = 2 * p->duration / 7;
    }
    fprintf(stderr, "Seeking to position %d\n", (int)(seek_point / GST_SECOND));
    gst_element_seek_simple(p->playbin.get(),
                            GST_FORMAT_TIME,
                            static_cast<GstSeekFlags>(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT),
                            seek_point);
    fprintf(stderr, "Waiting for seek to complete\n");
    gst_element_get_state(p->playbin.get(), nullptr, nullptr, GST_CLOCK_TIME_NONE);
    fprintf(stderr, "Done.\n");
}

void ThumbnailExtractor::save_screenshot(const std::string &filename) {
    // Retrieve sample from the playbin
    fprintf(stderr, "Requesting sample frame.\n");
    std::unique_ptr<GstSample, decltype(&gst_sample_unref)> sample(
        nullptr, gst_sample_unref);
    {
        std::unique_ptr<GstCaps, decltype(&gst_caps_unref)> desired_caps(
            gst_caps_new_simple(
                "video/x-raw",
                "format", G_TYPE_STRING, "RGB",
                "pixel-aspect-ratio", GST_TYPE_FRACTION, 1, 1,
                nullptr), gst_caps_unref);
        GstSample *s;
        g_signal_emit_by_name(p->playbin.get(), "convert-sample", desired_caps.get(), &s);
        sample.reset(s);
    }
    fprintf(stderr, "Got frame %p\n", sample.get());
    if (!sample) {
        throw std::runtime_error("Could not retrieve screenshot");
    }

    // Construct a pixbuf from the sample
    fprintf(stderr, "Creating pixbuf from sample\n");
    unique_gobj<GdkPixbuf> image;
    {
        GstCaps *sample_caps = gst_sample_get_caps(sample.get());
        if (!sample_caps) {
            throw std::runtime_error("Could not retrieve caps for sample buffer");
        }
        GstStructure *sample_struct = gst_caps_get_structure(sample_caps, 0);
        int width = 0, height = 0;
        gst_structure_get_int(sample_struct, "width", &width);
        gst_structure_get_int(sample_struct, "height", &height);
        if (width <= 0 || height <= 0) {
            throw std::runtime_error("Could not retrieve image dimensions");
        }

        GstBuffer *buffer = gst_sample_get_buffer(sample.get());
        GstMapInfo info;
        gst_buffer_map(buffer, &info, GST_MAP_READ);
        image.reset(gdk_pixbuf_new_from_data(
                        info.data, GDK_COLORSPACE_RGB,
                        FALSE, 8, width, height, GST_ROUND_UP_4(width *3),
                        nullptr, nullptr));
        gst_buffer_unmap(buffer, &info);
    }

    fprintf(stderr, "Saving pixbuf to jpeg\n");
    GError *error = nullptr;
    if (!gdk_pixbuf_save(image.get(), filename.c_str(),
                         "jpeg", &error, nullptr)) {
        std::string message(error->message);
        g_error_free(error);
        throw std::runtime_error(message);
    }
    fprintf(stderr, "Done.\n");
}


int main(int argc, char **argv) {
    gst_init(&argc, &argv);

    ThumbnailExtractor e;
    e.set_uri(argv[1]);
    e.seek_sample_frame();
    e.save_screenshot(argv[2]);
    return 0;
}
