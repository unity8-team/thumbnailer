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

#include <internal/raii.h>
#include <internal/thumbnailer.h>

#include <cstdio>
#include <stdexcept>
#include <sys/types.h>
#include <fcntl.h>

using namespace unity::thumbnailer::internal;

int main(int argc, char** argv)
{
    Thumbnailer t;
    if (argc != 2)
    {
        printf("%s <file name>\n", argv[0]);
        return 1;
    }
    std::string ifilename(argv[1]);
    FdPtr fd(open(ifilename.c_str(), O_RDONLY), do_close);
    std::string ofilename = t.get_thumbnail(ifilename, fd.get(), QSize(256, 256))->thumbnail();
    if (ofilename.empty())
    {
        printf("Thumbnail could not be generated.\n");
    }
    else
    {
        printf("Thumbnail file is %s.\n", ofilename.c_str());
    }
    return 0;
}
