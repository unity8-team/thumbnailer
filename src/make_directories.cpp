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
 * Authored by: MichiHenning <michi@canonical.com>
 */


#include <internal/make_directories.h>

#include <boost/filesystem.hpp>

using namespace std;

// Recursively create the directories in path, setting permissions to the specified mode
// (regardless of the setting of umask). If one or more directories already exist, they
// are left unchanged (including their permissions). If a directory cannot be created,
// fail silently.

void make_directories(string const& path_name, mode_t mode)
{
    using namespace boost::filesystem;

    // We can't use boost::create_directories() here because that does not allow control over permissions.
    auto abs = absolute(path_name);
    path path_so_far = "";
    path::iterator it = abs.begin();
    ++it; // No point in trying to create /
    while (it != abs.end())
    {
        path_so_far += "/";
        path_so_far += *it++;
        string p = path_so_far.native();
        if (mkdir(p.c_str(), mode) != 0)
        {
            if (errno == EEXIST)
            {
                continue;
            }
            return;  // No point in continuing, we'd fail on all subsequent iterations.
        }
        // We just created the dir, make sure it has the requested permissions,
        // not the permissions modified by umask.
        chmod(p.c_str(), mode);
    }
}
