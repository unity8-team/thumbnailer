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

#include<thumbnailer.h>
#include<testsetup.h>
#include<cassert>
#include<unistd.h>

using namespace std;

bool file_exists(const string &s) {
    FILE *f = fopen(s.c_str(), "r");
    if(f) {
        fclose(f);
        return true;
    }
    return false;
}

void trivial_test() {
    Thumbnailer tn;
}

void image_test() {
    Thumbnailer tn;
    string imfile(TESTDATADIR);
    imfile += "/testimage.jpg";
    string thumbfile = tn.get_thumbnail(imfile, TN_SIZE_SMALL);
    unlink(thumbfile.c_str());
    assert(!file_exists(thumbfile));
    string thumbfile2 = tn.get_thumbnail(imfile, TN_SIZE_SMALL);
    assert(thumbfile == thumbfile2);
    assert(file_exists(thumbfile));
}

void size_test() {
    Thumbnailer tn;
    string imfile(TESTDATADIR);
    imfile += "/testimage.jpg";
    string thumbfile = tn.get_thumbnail(imfile, TN_SIZE_SMALL);
    string thumbfile2 = tn.get_thumbnail(imfile, TN_SIZE_LARGE);
    assert(!thumbfile.empty());
    assert(!thumbfile2.empty());
    assert(thumbfile != thumbfile2);
}

int main() {
#ifdef NDEBUG
    fprintf(stderr, "NDEBUG defined, tests will not work.\n");
    return 1;
#else
    trivial_test();
    image_test();
    size_test();
    return 0;
#endif
}
