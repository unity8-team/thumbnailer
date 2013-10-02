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

#include<thumbnailer.h>
#include<testsetup.h>
#include<cassert>
#include<unistd.h>
#include<gdk-pixbuf/gdk-pixbuf.h>

#define TESTIMAGE TESTDATADIR "/testimage.jpg"
#define TESTVIDEO TESTDATADIR "/testvideo.ogg"

using namespace std;

bool file_exists(const string &s) {
    FILE *f = fopen(s.c_str(), "r");
    if(f) {
        fclose(f);
        return true;
    }
    return false;
}

void copy_file(const string &src, const string &dst) {
    FILE* f = fopen(src.c_str(), "r");
    assert(f);
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);

    char* buf = new char[size];

    fseek(f, 0, SEEK_SET);
    assert(fread(buf, 1, size, f) == size);
    fclose(f);

    f = fopen(dst.c_str(), "w");
    assert(f);
    assert(fwrite(buf, 1, size, f) == size);
    fclose(f);
    delete[] buf;
}

void trivial_test() {
    Thumbnailer tn;
}

void file_test(Thumbnailer &tn, string &ifile) {
    int w, h;
    assert(file_exists(ifile));
    string thumbfile = tn.get_thumbnail(ifile, TN_SIZE_SMALL);
    unlink(thumbfile.c_str());
    assert(!file_exists(thumbfile));
    string thumbfile2 = tn.get_thumbnail(ifile, TN_SIZE_SMALL);
    assert(thumbfile == thumbfile2);
    assert(file_exists(thumbfile));
    assert(gdk_pixbuf_get_file_info(thumbfile.c_str(), &w, &h));
    assert(w <= 128);
    assert(h <= 128);
}

void image_test() {
    Thumbnailer tn;
    string imfile(TESTIMAGE);
    file_test(tn, imfile);
}

void video_test() {
    Thumbnailer tn;
    string videofile(TESTVIDEO);
    file_test(tn, videofile);
}

void video_original_test() {
    Thumbnailer tn;
    int w, h;
    string videofile(TESTVIDEO);
    string origsize = tn.get_thumbnail(videofile, TN_SIZE_ORIGINAL);
    assert(file_exists(origsize));
    assert(gdk_pixbuf_get_file_info(origsize.c_str(), &w, &h));
    assert(w == 1920);
    assert(h == 1080);
}


void size_test() {
    Thumbnailer tn;
    int w, h;
    string imfile(TESTIMAGE);
    string thumbfile = tn.get_thumbnail(imfile, TN_SIZE_SMALL);
    string thumbfile2 = tn.get_thumbnail(imfile, TN_SIZE_LARGE);
    assert(!thumbfile.empty());
    assert(!thumbfile2.empty());
    assert(thumbfile != thumbfile2);
    assert(gdk_pixbuf_get_file_info(thumbfile.c_str(), &w, &h));
    assert(w == 128);
    assert(h <= 128);
    assert(gdk_pixbuf_get_file_info(thumbfile2.c_str(), &w, &h));
    assert(w == 256);
    assert(h <= 256);
}

void delete_test() {
    Thumbnailer tn;
    string srcimg(TESTIMAGE);
    string workimage("working_image.jpg");
    copy_file(srcimg, workimage);
    assert(file_exists(workimage));
    string thumbfile = tn.get_thumbnail(workimage, TN_SIZE_SMALL);
    string thumbfile2 = tn.get_thumbnail(workimage, TN_SIZE_LARGE);
    assert(file_exists(thumbfile));
    assert(file_exists(thumbfile2));
    unlink(workimage.c_str());
    string tmp = tn.get_thumbnail(workimage, TN_SIZE_SMALL);
    assert(tmp.empty());
    assert(!file_exists(thumbfile));
    assert(!file_exists(thumbfile2));
}

int main() {
#ifdef NDEBUG
    fprintf(stderr, "NDEBUG defined, tests will not work.\n");
    return 1;
#else
    trivial_test();
    image_test();
    video_test();
    video_original_test();
    size_test();
    delete_test();
    return 0;
#endif
}
