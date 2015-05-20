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

#include <internal/thumbnailer.h>

#include <QDBusArgument>
#include <QDBusContext>
#include <QDateTime>

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
    qlonglong size_in_bytes;
    qlonglong max_size_in_bytes;
    qlonglong hits;
    qlonglong misses;
    qlonglong hits_since_last_miss;
    qlonglong misses_since_last_hit;
    qlonglong longest_hit_run;
    qlonglong longest_miss_run;
    qlonglong ttl_evictions;
    qlonglong lru_evictions;
    QDateTime most_recent_hit_time;
    QDateTime most_recent_miss_time;
    QDateTime longest_hit_run_time;
    QDateTime longest_miss_run_time;
    QList<uint32_t> histogram;
};

struct AllStats
{
    CacheStats full_size_stats;
    CacheStats thumbnail_stats;
    CacheStats failure_stats;
};

class AdminInterface : public QObject, protected QDBusContext
{
    Q_OBJECT
public:
    AdminInterface(std::shared_ptr<unity::thumbnailer::internal::Thumbnailer> const& thumbnailer,
                   QObject* parent = nullptr)
        : QObject(parent)
        , thumbnailer_(thumbnailer)
    {
    }
    ~AdminInterface() = default;

    AdminInterface(AdminInterface const&) = delete;
    AdminInterface& operator=(AdminInterface&) = delete;

public Q_SLOTS:
    AllStats Stats();

private:
    std::shared_ptr<unity::thumbnailer::internal::Thumbnailer> const& thumbnailer_;
};

}  // namespace service

}  // namespace thumbnailer

}  // namespace unity

Q_DECLARE_METATYPE(unity::thumbnailer::service::AllStats)

QDBusArgument& operator<<(QDBusArgument& arg, unity::thumbnailer::service::CacheStats const& s);
QDBusArgument const& operator>>(QDBusArgument const& arg, unity::thumbnailer::service::CacheStats& s);

QDBusArgument& operator<<(QDBusArgument& arg, unity::thumbnailer::service::AllStats const& s);
QDBusArgument const& operator>>(QDBusArgument const& arg, unity::thumbnailer::service::AllStats& s);
