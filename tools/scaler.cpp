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

#if 0
#include<internal/imagescaler.h>
#include<cstdio>
#include<gst/gst.h>
#include<stdexcept>
#endif

int main(int argc, char **argv) {
#if 0
    ImageScaler sc;
    if(argc != 3) {
        fprintf(stderr, "%s <inputfile> <outputfile>\n", argv[0]);
    }
    try {
        std::string ifilename(argv[1]);
        std::string ofilename(argv[2]);
        sc.scale(ifilename, ofilename, 256, ifilename);
    } catch(std::runtime_error &e) {
        printf("Failed: %s\n", e.what());
        return 1;
    }
#endif
    return 0;
}
