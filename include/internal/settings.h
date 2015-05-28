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
 * Authored by: James Henstridge <james.henstridge@canonical.com>
 */

#pragma once

#include <internal/gobj_memory.h>

#include <string>

typedef struct _GSettings GSettings;

namespace unity
{

namespace thumbnailer
{

namespace internal
{

class Settings
{
public:
    Settings();
    ~Settings();

    std::string art_api_key() const;
    int full_size_cache_size() const;
    int thumbnail_cache_size() const;
    int failure_cache_size() const;

private:
    std::string get_string(std::string const& key, std::string const& default_value="") const;
    int get_int(std::string const& key, int default_value=0) const;

    gobj_ptr<GSettings> settings_;
};

}  // namespace internal

}  // namespace thumbnailer

}  // namespace unity
