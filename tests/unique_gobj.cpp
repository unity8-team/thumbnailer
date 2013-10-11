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
    gobj_unique<GdkPixbuf> basic(gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, 640, 480));
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
    gobj_unique<GdkPixbuf> u1(pb1);
    gobj_unique<GdkPixbuf> u2(pb2);

    assert(u1 != u2);
    assert(!(u1 == u2));
    assert(u1 < u2);
    assert(!(u2 < u1));
    assert(!(u1 == u2));
    assert(!(u2 == u1));
    assert(u1 <= u2);
    assert(!(u2 <= u1));

}

int main() {
#ifdef NDEBUG
    fprintf(stderr, "NDEBUG defined, tests will not work.\n");
    return 1;
#else
    trivial_test();
    compare_test();
    return 0;
#endif
}
