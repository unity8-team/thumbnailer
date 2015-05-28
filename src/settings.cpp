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

#include <internal/settings.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wcast-qual"
#include <gio/gio.h>
#pragma GCC diagnostic pop

#include <QDebug>

#include <memory>

using namespace std;

namespace
{
char const THUMBNAILER_SCHEMA[] = "com.canonical.Unity.Thumbnailer";
}

namespace unity
{

namespace thumbnailer
{

namespace internal
{

Settings::Settings()
{
    GSettingsSchemaSource* src = g_settings_schema_source_get_default();
    unique_ptr<GSettingsSchema, decltype(&g_settings_schema_unref)>
        schema(g_settings_schema_source_lookup(src, THUMBNAILER_SCHEMA, true),
               &g_settings_schema_unref);
    if (schema)
    {
        settings_.reset(g_settings_new(THUMBNAILER_SCHEMA));
    }
    else
    {
        qCritical() << "The schema" << THUMBNAILER_SCHEMA << "is missing"; // LCOV_EXCL_LINE
    }
}

Settings::~Settings() = default;

string Settings::art_api_key() const
{
    return get_string("dash-ubuntu-com-key");
}

int Settings::full_size_cache_size() const
{
    return get_int("full-size-cache-size", 50);
}

int Settings::thumbnail_cache_size() const
{
    return get_int("thumbnail-cache-size", 100);
}

int Settings::failure_cache_size() const
{
    return get_int("failure-cache-size", 2);
}

string Settings::get_string(string const& key, string const& default_value) const
{
    if (!settings_) return default_value;

    char *value = g_settings_get_string(settings_.get(), key.c_str());
    if (value)
    {
        string result = value;
        g_free(value);
        return result;
    }
    return default_value;
}

int Settings::get_int(string const& key, int default_value) const
{
    if (!settings_) return default_value;
    return g_settings_get_int(settings_.get(), key.c_str());
}


}  // namespace internal

}  // namespace thumbnailer

}  // namespace unity
