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
#include<gdk-pixbuf/gdk-pixbuf.h>
#include<cassert>

using namespace std;

void trivial_test() {
    unique_gobj<GdkPixbuf> basic(gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, 640, 480));
    assert(basic);
    assert(gdk_pixbuf_get_width(basic.get()) == 640);
    assert(gdk_pixbuf_get_height(basic.get()) == 480);
}

void compare_test() {
    GdkPixbuf *pb1 = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, 640, 480);
    GdkPixbuf *pb2 = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, 640, 480);
    if(pb1 > pb2) {
        GdkPixbuf *tmp;
        tmp = pb1;
        pb1 = pb2;
        pb2 = tmp;
    }
    assert(pb1 < pb2);
    unique_gobj<GdkPixbuf> u1(pb1);
    unique_gobj<GdkPixbuf> u2(pb2);

    assert(u1 != u2);
    assert(!(u1 == u2));
    assert(u1 < u2);
    assert(!(u2 < u1));
    assert(!(u1 == u2));
    assert(!(u2 == u1));
    assert(u1 <= u2);
    assert(!(u2 <= u1));
}

// This is its own thing due to need to avoid double release.

void equality_test() {
    GdkPixbuf *pb = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, 640, 480);
    unique_gobj<GdkPixbuf> u1(pb);
    unique_gobj<GdkPixbuf> u2(pb);
    assert(u1 == u2);
    assert(!(u1 != u2));
}

void release_test() {
    GdkPixbuf *pb = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, 640, 480);
    unique_gobj<GdkPixbuf> u(pb);
    assert(u);
    assert(pb == u.release());
    assert(!u);
    g_object_unref(pb);
}

void sub_func(GdkPixbuf *pb) {
    assert(G_OBJECT(pb)->ref_count == 2);
    unique_gobj<GdkPixbuf> u(pb);
    assert(G_OBJECT(pb)->ref_count == 2);
    // Now it dies and refcount is reduced.
}

void refcount_test() {
    GdkPixbuf *pb = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, 640, 480);
    assert(G_OBJECT(pb)->ref_count == 1);
    g_object_ref(G_OBJECT(pb));
    sub_func(pb);
    assert(G_OBJECT(pb)->ref_count == 1);
    g_object_unref(G_OBJECT(pb));
}

int main() {
#ifdef NDEBUG
    fprintf(stderr, "NDEBUG defined, tests will not work.\n");
    return 1;
#else
    trivial_test();
    compare_test();
    equality_test();
    release_test();
    refcount_test();
    return 0;
#endif
}
