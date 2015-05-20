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

#pragma once

#include <memory>

#include <QDBusArgument>
#include <QDBusContext>
#include <QObject>
#include <QSize>
#include <QString>

namespace unity
{

namespace thumbnailer
{

namespace service
{

struct CacheStats
{
    QString cache_path;
    uint32_t policy;
    qlonglong size;
#if 0
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
#endif
};

struct AllStats
{
    CacheStats full_size_stats;
    CacheStats thumbnail_stats;
    CacheStats failure_stats;
};

struct AdminInterfacePrivate;

class AdminInterface : public QObject, protected QDBusContext
{
    Q_OBJECT
public:
    explicit AdminInterface(QObject* parent = nullptr);
    ~AdminInterface();

    AdminInterface(AdminInterface const&) = delete;
    AdminInterface& operator=(AdminInterface&) = delete;

public Q_SLOTS:
    AllStats Stats();

private:
    std::unique_ptr<AdminInterfacePrivate> p;
};

}  // namespace service

}  // namespace thumbnailer

}  // namespace unity

Q_DECLARE_METATYPE(unity::thumbnailer::service::CacheStats)
Q_DECLARE_METATYPE(unity::thumbnailer::service::AllStats)

QDBusArgument& operator<<(QDBusArgument& arg, unity::thumbnailer::service::CacheStats const& cache_stats);
QDBusArgument const& operator>>(QDBusArgument const& arg, unity::thumbnailer::service::CacheStats& cache_stats);

QDBusArgument& operator<<(QDBusArgument& arg, unity::thumbnailer::service::AllStats const& all_stats);
QDBusArgument const& operator>>(QDBusArgument const& arg, unity::thumbnailer::service::AllStats& all_stats);
