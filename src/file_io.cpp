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

#include <boost/filesystem.hpp>

#include <cstring>

#include <fcntl.h>
#include <sys/stat.h>

using namespace unity::thumbnailer::internal;
using namespace std;

// Read entire file and return contents as a string.

string read_file(string const& filename)
{
    int fd = open(filename.c_str(), O_RDONLY | O_CLOEXEC);
    if (fd == -1)
    {
        throw runtime_error("read_file(): cannot open \"" + filename + "\": " + safe_strerror(errno));
    }
    FdPtr fd_ptr(fd, do_close);

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

namespace
{
    
static string tmp_dir = []
{
    char const* dirp = getenv("TMPDIR");
    string dir = dirp ? dirp : "/tmp";
    return dir;
}();

}  // namespace

// Atomically write contents to filename.

void write_file(string const& filename, string const& contents)
{
    using namespace boost::filesystem;

    path abs_path = absolute(filename);
    path dir = abs_path.parent_path();

    string tmp_path = dir.native() + "/thumbnailer.XXXXXX";
    int fd = mkstemp(&tmp_path[0]);
    if (fd == -1)
    {
        string s = string("write_file(): mkstemp() failed for " + tmp_path + ": ") + safe_strerror(errno);
        throw runtime_error(s);
    }

    {
        FdPtr fd_ptr(fd, do_close);

        int rc = write(fd_ptr.get(), &contents[0], contents.size());
        if (rc == -1)
        {
            throw runtime_error("write_file(): cannot write to \"" + filename + "\": " + safe_strerror(errno)); // LCOV_EXCL_LINE
        }
        if (string::size_type(rc) != contents.size())
        {
            throw runtime_error("write_file(): short write for \"" + filename + "\""); // LCOV_EXCL_LINE
        }
    }  // Close tmp file

    if (rename(tmp_path.c_str(), filename.c_str()) == -1)
    {
        // LCOV_EXCL_START
        unlink(tmp_path.c_str());
        throw runtime_error("write_file(): cannot rename " + tmp_path + " to " + filename + ": " + safe_strerror(errno));
        // LCOV_EXCL_STOP
    }
}

string create_tmp_filename()
{
    string tmp_path = tmp_dir + "/thumbnailer.XXXXXX";
    int fd = mkstemp(&tmp_path[0]);
    if (fd == -1)
    {
        // LCOV_EXCL_START
        string s = string("create_tmp_filename(): mkstemp() failed for " + tmp_path + ": ") + safe_strerror(errno);
        throw runtime_error(s);
        // LCOV_EXCL_STOP
    }
    close(fd);
    return tmp_path;
}
