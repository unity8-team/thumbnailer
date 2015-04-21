/*
 * Copyright (C) 2015 Canonical Ltd
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
 * Authored by: Michi Henning <michi.henning@canonical.com>
 */

#include <internal/file_io.h>

#include <internal/raii.h>
#include <internal/safe_strerror.h>

#include <cstring>

#include <fcntl.h>
#include <sys/stat.h>

using namespace unity::thumbnailer::internal;
using namespace std;

// Read entire file and return contents as a string.

string read_file(string const& filename)
{
    FdPtr fd_ptr(::open(filename.c_str(), O_RDONLY), do_close);
    if (fd_ptr.get() == -1)
    {
        throw runtime_error("read_file(): cannot open \"" + filename + "\": " + safe_strerror(errno));
    }

    struct stat st;
    if (fstat(fd_ptr.get(), &st) == -1)
    {
        throw runtime_error("read_file(): cannot fstat \"" + filename + "\": " + safe_strerror(errno)); // LCOV_EXCL_LINE
    }

    string contents;
    contents.reserve(st.st_size);
    contents.resize(st.st_size);
    int rc = read(fd_ptr.get(), &contents[0], st.st_size);
    if (rc == -1)
    {
        throw runtime_error("read_file(): cannot read from \"" + filename + "\": " + safe_strerror(errno)); // LCOV_EXCL_LINE
    }
    if (rc != st.st_size)
    {
        throw runtime_error("read_file(): short read for \"" + filename + "\""); // LCOV_EXCL_LINE
    }

    return contents;
}

// Write contents to filename.

void write_file(string const& filename, string const& contents)
{
    FdPtr fd_ptr(::open(filename.c_str(), O_CREAT | O_TRUNC | O_WRONLY, 0700), do_close);
    if (fd_ptr.get() == -1)
    {
        throw runtime_error("write_file(): cannot open \"" + filename + "\": " + safe_strerror(errno));
    }
    int rc = write(fd_ptr.get(), &contents[0], contents.size());
    if (rc == -1)
    {
        throw runtime_error("write_file(): cannot write to \"" + filename + "\": " + safe_strerror(errno)); // LCOV_EXCL_LINE
    }
    if (string::size_type(rc) != contents.size())
    {
        throw runtime_error("write_file(): short write for \"" + filename + "\""); // LCOV_EXCL_LINE
    }
}
