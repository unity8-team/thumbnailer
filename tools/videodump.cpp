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

#include<internal/videoscreenshotter.h>
#include<cstdio>
#include<gst/gst.h>
#include<stdexcept>

int main(int argc, char **argv) {
    gst_init(&argc, &argv);
    VideoScreenshotter ve;
    try {
        ve.extract(argv[1], argv[2]);
    } catch(std::runtime_error &e) {
        printf("Failed: %s\n", e.what());
        return 1;
    }
    return 0;
}
