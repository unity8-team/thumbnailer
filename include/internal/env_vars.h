/*
 * Copyright (C) 2015 Canonical Ltd.
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
 * Authored by: Michi Henning <michi@canonical.com>
 */

#pragma once

#include <internal/config.h>

#include <QString>

namespace unity
{

namespace thumbnailer
{

namespace internal
{

struct EnvVars
{
    static void set_snap_env();

    static int get_max_idle();
    static QString get_ubuntu_server_url();
    static QString get_util_dir();
    static int get_log_level();
    static std::string get_cache_dir();

    static constexpr char const* MAX_IDLE = "THUMBNAILER_MAX_IDLE";
    static constexpr int DFLT_MAX_IDLE = 30000;

    static constexpr char const* UBUNTU_SERVER_URL = "THUMBNAILER_UBUNTU_SERVER_URL";
    static constexpr char const* DFLT_UBUNTU_SERVER_URL = "https://dash.ubuntu.com";

    static constexpr char const* UTIL_DIR = "THUMBNAILER_UTIL_DIR";
    static constexpr char const* DFLT_UTIL_DIR = SHARE_PRIV_ABS;

    static constexpr char const* LOG_LEVEL = "THUMBNAILER_LOG_LEVEL";
};

}  // namespace internal

}  // namespace thumbnailer

}  // namespace unity
