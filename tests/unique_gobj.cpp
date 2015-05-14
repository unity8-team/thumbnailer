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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wcast-qual"

#include<internal/gobj_memory.h>
#include<gdk-pixbuf/gdk-pixbuf.h>
#include<gtest/gtest.h>

using namespace std;

TEST(Gobj_ptr, trivial) {
    gobj_ptr<GdkPixbuf> basic(gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, 640, 480));
    ASSERT_TRUE(bool(basic));
    ASSERT_TRUE(gdk_pixbuf_get_width(basic.get()) == 640);
    ASSERT_TRUE(gdk_pixbuf_get_height(basic.get()) == 480);
}

TEST(Gobj_ptr, compare) {
    GdkPixbuf *pb1 = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, 640, 480);
    GdkPixbuf *pb2 = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, 640, 480);
    if(pb1 > pb2) {
        std::swap(pb1, pb2);
    }
    ASSERT_TRUE(pb1 < pb2);
    gobj_ptr<GdkPixbuf> u1(pb1);
    gobj_ptr<GdkPixbuf> u2(pb2);

    ASSERT_TRUE(!(u1 == nullptr));
    ASSERT_TRUE(u1 != nullptr);
    ASSERT_TRUE(u1 != u2);
    ASSERT_TRUE(!(u1 == u2));
    ASSERT_TRUE(u1 < u2);
    ASSERT_TRUE(!(u2 < u1));
    ASSERT_TRUE(!(u1 == u2));
    ASSERT_TRUE(!(u2 == u1));
    ASSERT_TRUE(u1 <= u2);
    ASSERT_TRUE(!(u2 <= u1));
}

// This is its own thing due to need to avoid double release.

TEST(Gobj_ptr, equality) {
    GdkPixbuf *pb = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, 640, 480);
    gobj_ptr<GdkPixbuf> u1(pb);
    g_object_ref(G_OBJECT(pb));
    gobj_ptr<GdkPixbuf> u2(pb);
    ASSERT_TRUE(u1 == u2);
    ASSERT_TRUE(u2 == u1);
    ASSERT_TRUE(!(u1 != u2));
    ASSERT_TRUE(!(u2 != u1));
}

TEST(Gobj_ptr, release) {
    GdkPixbuf *pb = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, 640, 480);
    gobj_ptr<GdkPixbuf> u(pb);
    ASSERT_TRUE(u != nullptr);
    ASSERT_TRUE(u.get() != nullptr);
    ASSERT_TRUE(pb == u.release());
    ASSERT_TRUE(!u);
    ASSERT_TRUE(u.get() == nullptr);
    g_object_unref(pb);
}

void sub_func(GdkPixbuf *pb) {
    ASSERT_TRUE(G_OBJECT(pb)->ref_count == 2);
    gobj_ptr<GdkPixbuf> u(pb);
    ASSERT_TRUE(G_OBJECT(pb)->ref_count == 2);
    // Now it dies and refcount is reduced.
}

TEST(Gobj_ptr, refcount) {
    GdkPixbuf *pb = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, 640, 480);
    ASSERT_TRUE(G_OBJECT(pb)->ref_count == 1);
    g_object_ref(G_OBJECT(pb));
    sub_func(pb);
    ASSERT_TRUE(G_OBJECT(pb)->ref_count == 1);
    g_object_unref(G_OBJECT(pb));
}

TEST(Gobj_ptr, swap) {
    GdkPixbuf *pb1 = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, 640, 480);
    GdkPixbuf *pb2 = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, 640, 480);
    gobj_ptr<GdkPixbuf> u1(pb1);
    gobj_ptr<GdkPixbuf> u2(pb2);

    u1.swap(u2);
    ASSERT_TRUE(u1.get() == pb2);
    ASSERT_TRUE(u2.get() == pb1);

    std::swap(u1, u2);
    ASSERT_TRUE(u1.get() == pb1);
    ASSERT_TRUE(u2.get() == pb2);
}

TEST(Gobj_ptr, floating) {
    GdkPixbuf *pb = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, 640, 480);
    bool got_exception = false;
    g_object_force_floating(G_OBJECT(pb));
    try {
        gobj_ptr<GdkPixbuf> u(pb);
    } catch(const invalid_argument &c) {
        got_exception = true;
    }
    g_object_unref(G_OBJECT(pb));
    ASSERT_TRUE(got_exception);
}

TEST(Gobj_ptr, move) {
    GdkPixbuf *pb1 = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, 640, 480);
    GdkPixbuf *pb2 = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, 640, 480);
    g_object_ref(G_OBJECT(pb1));
    gobj_ptr<GdkPixbuf> u1(pb1);
    gobj_ptr<GdkPixbuf> u2(pb2);
    u1 = std::move(u2);
    ASSERT_TRUE(u1.get() == pb2);
    ASSERT_TRUE(!u2);
    ASSERT_TRUE(G_OBJECT(pb1)->ref_count == 1);
    g_object_unref(G_OBJECT(pb1));
}

TEST(Gobj_ptr, null) {
    GdkPixbuf *pb1 = NULL;
    GdkPixbuf *pb3 = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, 640, 480);
    gobj_ptr<GdkPixbuf> u1(pb1);
    gobj_ptr<GdkPixbuf> u2(nullptr);
    gobj_ptr<GdkPixbuf> u3(pb3);

    ASSERT_TRUE(!u1);
    ASSERT_TRUE(!u2);
    u3 = nullptr;
    ASSERT_TRUE(!u3);
}

TEST(Gobj_ptr, reset) {
    GdkPixbuf *pb1 = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, 640, 480);
    GdkPixbuf *pb2 = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, 640, 480);
    gobj_ptr<GdkPixbuf> u(pb1);

    u.reset(pb2);
    ASSERT_TRUE(u.get() == pb2);
    u.reset(nullptr);
    ASSERT_TRUE(!u);
}

TEST(Gobj_ptr, sizeoftest) {
    ASSERT_TRUE(sizeof(GdkPixbuf*) == sizeof(gobj_ptr<GdkPixbuf>));
}

TEST(Gobj_ptr, deleter) {
    gobj_ptr<GdkPixbuf> u1;
    ASSERT_TRUE(u1.get_deleter() == g_object_unref);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

#pragma GCC diagnostic pop
