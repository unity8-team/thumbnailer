/*
 * Copyright (C) 2017 Canonical Ltd.
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
 * Authored by: Michi Henning <michi.henning@canonical.com>
 */

#include <internal/env_vars.h>

#include <internal/config.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#include <glib.h>
#pragma GCC diagnostic pop
#include <QDebug>
#include <sstream>

using namespace std;

namespace unity
{

namespace thumbnailer
{

namespace internal
{

int EnvVars::get_max_idle()
{
    char const* c_idle_time = getenv(MAX_IDLE);
    if (c_idle_time)
    {
        std::string str_idle_time(c_idle_time);
        try
        {
            int env_value = std::stoi(str_idle_time);
            if (env_value < 1000)
            {
                std::ostringstream s;
                s << "Value for env variable THUMBNAILER_MAX_IDLE \"" << env_value << "\" must be >= 1000.";
                throw std::invalid_argument(s.str());
            }
            return env_value;
        }
        catch (std::exception& e)
        {
            std::ostringstream s;
            s << "Value for env variable THUMBNAILER_MAX_IDLE \"" << str_idle_time << "\" must be >= 1000.";
            throw std::invalid_argument(s.str());
        }
    }
    return DFLT_MAX_IDLE;
}

QString EnvVars::get_ubuntu_server_url()
{
    char const* override_url = getenv(UBUNTU_SERVER_URL);
    if (override_url && *override_url)
    {
        return override_url;
    }
    return DFLT_UBUNTU_SERVER_URL;
}

QString EnvVars::get_util_dir()
{
#ifdef SNAP_BUILD
    char const* snapdir = getenv("SNAP");
    if (!snapdir || !*snapdir)
    {
        throw std::invalid_argument("Environment variable SNAP is not set or empty.");
    }
    return QString(snapdir) + "/" + SHARE_PRIV_ABS;
#else
    char const* utildir = getenv(UTIL_DIR);
    if (utildir && *utildir)
    {
        return utildir;
    }
    return SHARE_PRIV_ABS;
#endif
}

int EnvVars::get_log_level()
{
    char const* level = getenv(LOG_LEVEL);
    if (level && *level)
    {
        int l;
        try
        {
            l = std::stoi(level);
        }
        catch (std::exception const& e)
        {
            qCritical() << "Environment variable" << LOG_LEVEL << "has invalid setting:" << level
                        << "(expected value in range 0..2) - variable ignored";
            return -1;
        }
        if (l < 0 || l > 2)
        {
            qCritical() << "Environment variable" << LOG_LEVEL << "has invalid setting:" << level
                        << "(expected value in range 0..2) - variable ignored";
            return -1;
        }
        return l;
    }
    return -1;
}

string EnvVars::get_cache_dir()
{
    string cache_dir = g_get_user_cache_dir();  // Always returns something, even HOME and XDG_CACHE_HOME are not set.

#ifdef SNAP_BUILD
    // When running in a snap, g_get_user_cache_dir() returns $SNAP_USER_DATA (not shared among snap versions),
    // but we want $SNAP_USER_COMMON, which is shared. PersistentCache automatically deals with versioning
    // changes in the database, so reverting to a snap with an earlier DB schema is safe.
    char const* user_common = getenv("SNAP_USER_COMMON");
    if (user_common && *user_common)
    {
        cache_dir = user_common;
    }
#endif

    return cache_dir;
}

}  // namespace internal

}  // namespace thumbnailer

}  // namespace unity
