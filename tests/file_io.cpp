/*
 * Copyright (C) 2015 Canonical Ltd.
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

#include <boost/algorithm/string.hpp>
#include <gtest/gtest.h>
#include <testsetup.h>

using namespace std;

TEST(file_io, read_write)
{
    string in_file = TESTDATADIR "/testimage.jpg";
    struct stat st;
    ASSERT_NE(-1, stat(in_file.c_str(), &st));

    string data = read_file(in_file);
    ASSERT_EQ(st.st_size, data.size());

    string out_file = TESTBINDIR "/testimage.jpg";
    write_file(out_file, data);

    string cmd = "cmp " + in_file + " " + out_file;
    int rc = system(cmd.c_str());
    EXPECT_EQ(0, rc);
}

TEST(file_io, tmp_filename)
{
    string tfn = create_tmp_filename();
    EXPECT_NE(string::npos, tfn.find("/thumbnailer."));
}

TEST(file_io, exceptions)
{
    try
    {
        read_file("no_such_file");
        FAIL();
    }
    catch (runtime_error const& e)
    {
        EXPECT_STREQ("read_file(): cannot open \"no_such_file\": No such file or directory", e.what());
    }

    string dir = TESTBINDIR "/dir";
    chmod(dir.c_str(), 0700);  // If dir exists, make sure it's writable.
    mkdir(dir.c_str(), 0700);  // If dir doesn't exist, create it.

    string out_file = dir + "/no_perm";
    write_file(out_file, "");

    chmod(dir.c_str(), 0500);  // Remove write permission on directory

    try
    {
        write_file(out_file, "");
        chmod(dir.c_str(), 0700);
        FAIL();
    }
    catch (runtime_error const& e)
    {
        chmod(dir.c_str(), 0700);
        string msg = e.what();
        EXPECT_TRUE(boost::starts_with(msg, "write_file(): mkstemp() failed for ")) << msg;
    }
}
