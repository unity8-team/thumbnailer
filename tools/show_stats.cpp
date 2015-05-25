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

#include "show_stats.h"

#include <iostream>

#include <inttypes.h>

using namespace std;

namespace unity
{

namespace thumbnailer
{

namespace tools
{

ShowStats::ShowStats(vector<string> const& args)
    : Action(args)
{
    if (args.size() > 4)
    {
        throw "too many arguments for stats command";
    }
    for (vector<string>::size_type i = 2; i < args.size(); ++i)
    {
        if (args[i] == "hist")
        {
            show_histogram_ = true;
        }
        else if (args[i] == "i")
        {
            show_image_stats_ = true;
            show_thumbnail_stats_ = false;
            show_failure_stats_ = false;
        }
        else if (args[i] == "t")
        {
            show_image_stats_ = false;
            show_thumbnail_stats_ = true;
            show_failure_stats_ = true;
        }
        else if (args[i] == "f")
        {
            show_image_stats_ = false;
            show_thumbnail_stats_ = false;
            show_failure_stats_ = true;
        }
        else
        {
            throw string("invalid argument for stats command: " ) + args[i];
        }
    }
}

ShowStats::~ShowStats() {}

void ShowStats::show_stats(service::CacheStats const& st)
{
    printf("    Path:                  %s\n", st.cache_path.toUtf8().data());
    char const* policy = st.policy ? "lru_ttl" : "lru_only";
    printf("    Policy:                %s\n", policy);
    printf("    Size:                  %" PRId64 "\n", int64_t(st.size));
    printf("    Size in bytes:         %" PRId64 "\n", int64_t(st.size_in_bytes));
    printf("    Max size in bytes:     %" PRId64 "\n", int64_t(st.max_size_in_bytes));
    printf("    Hits:                  %" PRId64 "\n", int64_t(st.hits));
    printf("    Misses:                %" PRId64 "\n", int64_t(st.misses));
    printf("    Hits since last miss:  %" PRId64 "\n", int64_t(st.hits_since_last_miss));
    printf("    Misses_since_last_hit: %" PRId64 "\n", int64_t(st.misses_since_last_hit));
    printf("    Longest hit run:       %" PRId64 "\n", int64_t(st.longest_hit_run));
    printf("    Longest miss run:      %" PRId64 "\n", int64_t(st.longest_miss_run));
    printf("    TTL evictions:         %" PRId64 "\n", int64_t(st.ttl_evictions));
    printf("    LRU evictions:         %" PRId64 "\n", int64_t(st.lru_evictions));
}

void ShowStats::run(DBusConnection& conn)
{
    qDBusRegisterMetaType<unity::thumbnailer::service::AllStats>();

    auto reply = conn.admin().Stats();
    reply.waitForFinished();
    if (!reply.isValid())
    {
        throw reply.error().message();
    }
    auto st = reply.value();
    if (show_image_stats_)
    {
        printf("%s\n", "Image cache:");
        show_stats(st.full_size_stats);
    }
    if (show_thumbnail_stats_)
    {
        printf("%s\n", "Thumbnail cache:");
        show_stats(st.thumbnail_stats);
    }
    if (show_failure_stats_)
    {
        printf("%s\n", "Failure cache:");
        show_stats(st.failure_stats);
    }
}

}  // namespace tools

}  // namespace thumbnailer

}  // namespace unity
