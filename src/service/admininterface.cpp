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

#include <iomanip>
#include <sstream>

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
// We figure out the epoch for both clocks and adjust if they differ.
// (On Linux, this should never be the case. Still, without the adjustment,
// the conversion is technically undefined.)

using namespace std::chrono;

static auto adjustment_ms = []
{
    // Arbitrary point in time, doesn't really matter what it is,
    // as long as it is after 1 Jan 1970.
    string const fixed_point = "Mar 15 16:45:17 1983";

    std::tm tm;
    memset(&tm, 0, sizeof(tm));
    strptime(fixed_point.c_str(), "%b %j %H:%M:%S %Y", &tm);
    auto tp = std::chrono::system_clock::from_time_t(std::mktime(&tm));
    auto system_msecs = duration_cast<milliseconds>(tp.time_since_epoch()).count();

    QDateTime qdt = QDateTime::fromString(QString::fromStdString(fixed_point), "MMM d hh:mm:ss yyyy");
    auto qt_msecs = qdt.toMSecsSinceEpoch();

    int64_t adjust_ms = system_msecs - qt_msecs;
    return adjust_ms;
}();

QDateTime to_date_time(chrono::system_clock::time_point tp)
{
    return QDateTime::fromMSecsSinceEpoch(duration_cast<milliseconds>(tp.time_since_epoch()).count() - adjustment_ms);
}

QList<quint32> to_list(core::PersistentCacheStats::Histogram const& histogram)
{
    QList<quint32> l;
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
        quint32(st.policy()),
        qint64(st.size()),
        qint64(st.size_in_bytes()),
        qint64(st.max_size_in_bytes()),
        qint64(st.hits()),
        qint64(st.misses()),
        qint64(st.hits_since_last_miss()),
        qint64(st.misses_since_last_hit()),
        qint64(st.longest_hit_run()),
        qint64(st.longest_miss_run()),
        qint64(st.ttl_evictions()),
        qint64(st.lru_evictions()),
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
