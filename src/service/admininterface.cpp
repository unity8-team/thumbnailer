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

#include <QDateTime>

using namespace std;
using namespace unity::thumbnailer::internal;

namespace unity
{

namespace thumbnailer
{

namespace service
{

struct AdminInterfacePrivate
{
#if 0
    std::shared_ptr<Thumbnailer> thumbnailer = std::make_shared<Thumbnailer>();
    std::map<Handler*, std::unique_ptr<Handler>> requests;
    std::shared_ptr<QThreadPool> check_thread_pool = std::make_shared<QThreadPool>();
    std::shared_ptr<QThreadPool> create_thread_pool = std::make_shared<QThreadPool>();
#endif
};

AdminInterface::AdminInterface(QObject* parent)
    : QObject(parent)
    , p(new AdminInterfacePrivate)
{
}

AdminInterface::~AdminInterface()
{
}

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

QList<QVariant> to_list(core::PersistentCacheStats::Histogram const& histogram)
{
    QList<QVariant> l;
    for (auto c : histogram)
    {
        l.append(QVariant(c));
    }
    return l;
}

QVariantMap to_variant_map(core::PersistentCacheStats const& st)
{
    QVariantMap m;
    m.insert("cache_path", QVariant(QString(st.cache_path().c_str())));
    m.insert("policy", QVariant(static_cast<uint32_t>(st.policy())));
    m.insert("size", QVariant(static_cast<qlonglong>(st.size())));
    m.insert("size_in_bytes", QVariant(static_cast<qlonglong>(st.size_in_bytes())));
    m.insert("max_size_in_bytes", QVariant(static_cast<qlonglong>(st.max_size_in_bytes())));
    m.insert("hits", QVariant(static_cast<qlonglong>(st.hits())));
    m.insert("misses", QVariant(static_cast<qlonglong>(st.misses())));
    m.insert("hits_since_last_miss", QVariant(static_cast<qlonglong>(st.hits_since_last_miss())));
    m.insert("misses_since_last_hit", QVariant(static_cast<qlonglong>(st.misses_since_last_hit())));
    m.insert("longest_hit_run", QVariant(static_cast<qlonglong>(st.longest_hit_run())));
    m.insert("longest_miss_run", QVariant(static_cast<qlonglong>(st.longest_miss_run())));
    m.insert("ttl_evictions", QVariant(static_cast<qlonglong>(st.ttl_evictions())));
    m.insert("lru_evictions", QVariant(static_cast<qlonglong>(st.lru_evictions())));
    m.insert("most_recent_hit_time", QVariant(to_date_time(st.most_recent_hit_time())));
    m.insert("most_recent_miss_time", QVariant(to_date_time(st.most_recent_miss_time())));
    m.insert("longest_hit_run_time", QVariant(to_date_time(st.longest_hit_run_time())));
    m.insert("longest_miss_run_time", QVariant(to_date_time(st.longest_miss_run_time())));
    m.insert("histogram", QVariant(to_list(st.histogram())));
    return m;
}

}  // namespace

unity::thumbnailer::service::AllStats AdminInterface::Stats()
{
#if 0
    auto st = p->thumbnailer->stats();
    QVariantMap m;
    m.insert("full_size_stats", QVariant(to_variant_map(st.full_size_stats)));
    m.insert("thumbnail_stats", QVariant(to_variant_map(st.thumbnail_stats)));
    m.insert("failure_stats", QVariant(to_variant_map(st.failure_stats)));
    return m;
#endif
    return AllStats();
}

}  // namespace service

}  // namespace thumbnailer

}  // namespace unity

QDBusArgument& operator<<(QDBusArgument& arg, unity::thumbnailer::service::CacheStats const& cache_stats)
{
    arg.beginStructure();
    arg << cache_stats.cache_path
        << cache_stats.policy
        << cache_stats.size;
    arg.endStructure();
    return arg;
}

QDBusArgument const& operator>>(QDBusArgument const& arg, unity::thumbnailer::service::CacheStats& cache_stats)
{
    arg.beginStructure();
    arg >> cache_stats.cache_path
        >> cache_stats.policy
        >> cache_stats.size;
    arg.endStructure();
    return arg;
}

QDBusArgument& operator<<(QDBusArgument& arg, unity::thumbnailer::service::AllStats const& all_stats)
{
    arg.beginStructure();
    arg << all_stats.full_size_stats
        << all_stats.thumbnail_stats
        << all_stats.failure_stats;
    arg.endStructure();
    return arg;
}

QDBusArgument const& operator>>(QDBusArgument const& arg, unity::thumbnailer::service::AllStats& all_stats)
{
    arg.beginStructure();
    arg >> all_stats.full_size_stats
        >> all_stats.thumbnail_stats
        >> all_stats.failure_stats;
    arg.endStructure();
    return arg;
}
