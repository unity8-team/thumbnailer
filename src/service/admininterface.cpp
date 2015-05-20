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

#include "admininterface.h"

#include <internal/thumbnailer.h>

using namespace std;
using namespace unity::thumbnailer::internal;

namespace unity
{

namespace thumbnailer
{

namespace service
{

namespace
{

// Conversion to QDateTime is somewhat awkward because system_clock
// is not guaranteed to use the same epoch as QDateTime.
// (The C++ standard leaves the epoch time point undefined.)
// We figure out the epoch for both clocks and adjust
// if they differ by more than a day, to allow for the (very
// unlikely) case of getting a SIGSTOP in between the calls to
// retrieve the current time for each clock. If we are suspended
// for more than a day at just that point, that's too bad...

using namespace std::chrono;

static auto adjustment_ms = []
{
    auto qt_msecs = QDateTime::currentMSecsSinceEpoch();
    auto system_msecs = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    int64_t adjust_ms = 0;
    auto diff_in_hours = duration_cast<hours>(milliseconds(abs(system_msecs - qt_msecs)));
    if (diff_in_hours.count() > 24)
    {
        adjust_ms = system_msecs - qt_msecs;
    }
    return adjust_ms;
}();

QDateTime to_date_time(chrono::system_clock::time_point tp)
{
    return QDateTime::fromMSecsSinceEpoch(duration_cast<milliseconds>(tp.time_since_epoch()).count() - adjustment_ms);
}

QList<uint32_t> to_list(core::PersistentCacheStats::Histogram const& histogram)
{
    QList<uint32_t> l;
    for (auto c : histogram)
    {
        l.append(c);
    }
    return l;
}

CacheStats to_cache_stats(core::PersistentCacheStats const& st)
{
    return
    {
        QString(st.cache_path().c_str()),
        static_cast<uint32_t>(st.policy()),
        static_cast<qlonglong>(st.size()),
        static_cast<qlonglong>(st.size_in_bytes()),
        static_cast<qlonglong>(st.max_size_in_bytes()),
        static_cast<qlonglong>(st.hits()),
        static_cast<qlonglong>(st.misses()),
        static_cast<qlonglong>(st.hits_since_last_miss()),
        static_cast<qlonglong>(st.misses_since_last_hit()),
        static_cast<qlonglong>(st.longest_hit_run()),
        static_cast<qlonglong>(st.longest_miss_run()),
        static_cast<qlonglong>(st.ttl_evictions()),
        static_cast<qlonglong>(st.lru_evictions()),
        to_date_time(st.most_recent_hit_time()),
        to_date_time(st.most_recent_miss_time()),
        to_date_time(st.longest_hit_run_time()),
        to_date_time(st.longest_miss_run_time()),
        to_list(st.histogram())
    };
}

}  // namespace

unity::thumbnailer::service::AllStats AdminInterface::Stats()
{
    auto const st = thumbnailer_->stats();
    AllStats all;
    all.full_size_stats = to_cache_stats(st.full_size_stats);
    all.thumbnail_stats = to_cache_stats(st.thumbnail_stats);
    all.failure_stats = to_cache_stats(st.failure_stats);
    return all;
}

}  // namespace service

}  // namespace thumbnailer

}  // namespace unity

QDBusArgument& operator<<(QDBusArgument& arg, unity::thumbnailer::service::CacheStats const& s)
{
    arg.beginStructure();
    arg << s.cache_path
        << s.policy
        << s.size
        << s.size_in_bytes
        << s.max_size_in_bytes
        << s.hits
        << s.misses
        << s.hits_since_last_miss
        << s.misses_since_last_hit
        << s.longest_hit_run
        << s.longest_miss_run
        << s.ttl_evictions
        << s.lru_evictions
        << s.most_recent_hit_time
        << s.most_recent_miss_time
        << s.longest_hit_run_time
        << s.longest_miss_run_time
        << s.histogram;
        arg.endStructure();
    return arg;
}

QDBusArgument const& operator>>(QDBusArgument const& arg, unity::thumbnailer::service::CacheStats& s)
{
    arg.beginStructure();
    arg >> s.cache_path
        >> s.policy
        >> s.size
        >> s.size_in_bytes
        >> s.max_size_in_bytes
        >> s.hits
        >> s.misses
        >> s.hits_since_last_miss
        >> s.misses_since_last_hit
        >> s.longest_hit_run
        >> s.longest_miss_run
        >> s.ttl_evictions
        >> s.lru_evictions
        >> s.most_recent_hit_time
        >> s.most_recent_miss_time
        >> s.longest_hit_run_time
        >> s.longest_miss_run_time
        >> s.histogram;
    arg.endStructure();
    return arg;
}

QDBusArgument& operator<<(QDBusArgument& arg, unity::thumbnailer::service::AllStats const& s)
{
    arg.beginStructure();
    arg << s.full_size_stats
        << s.thumbnail_stats
        << s.failure_stats;
    arg.endStructure();
    return arg;
}

QDBusArgument const& operator>>(QDBusArgument const& arg, unity::thumbnailer::service::AllStats& s)
{
    arg.beginStructure();
    arg >> s.full_size_stats
        >> s.thumbnail_stats
        >> s.failure_stats;
    arg.endStructure();
    return arg;
}
