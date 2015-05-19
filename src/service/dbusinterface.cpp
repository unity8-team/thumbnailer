/*
 * Copyright (C) 2014 Canonical, Ltd.
 *
 * Authors:
 *    James Henstridge <james.henstridge@canonical.com>
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of version 3 of the GNU General Public License as published
 * by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "dbusinterface.h"
#include "handler.h"

#include <internal/safe_strerror.h>
#include <internal/thumbnailer.h>

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDateTime>
#include <QDebug>
#include <QThreadPool>
#include <unity/Exception.h>

#include <chrono>
#include <map>
#include <sstream>

using namespace std;
using namespace unity::thumbnailer::internal;

namespace
{
char const ART_ERROR[] = "com.canonical.Thumbnailer.Error.Failed";
}

namespace unity
{
namespace thumbnailer
{
namespace service
{

struct DBusInterfacePrivate
{
    std::shared_ptr<Thumbnailer> thumbnailer = std::make_shared<Thumbnailer>();
    std::map<Handler*, std::unique_ptr<Handler>> requests;
    std::shared_ptr<QThreadPool> check_thread_pool = std::make_shared<QThreadPool>();
    std::shared_ptr<QThreadPool> create_thread_pool = std::make_shared<QThreadPool>();
};

DBusInterface::DBusInterface(QObject* parent)
    : QObject(parent)
    , p(new DBusInterfacePrivate)
{
}

DBusInterface::~DBusInterface()
{
}

QDBusUnixFileDescriptor DBusInterface::GetAlbumArt(QString const& artist,
                                                   QString const& album,
                                                   QSize const& requestedSize)
{
    qDebug() << "Look up cover art for" << artist << "/" << album << "at size" << requestedSize;
    auto request = p->thumbnailer->get_album_art(artist.toStdString(), album.toStdString(), requestedSize);
    queueRequest(new Handler(connection(), message(), p->check_thread_pool, p->create_thread_pool, std::move(request)));
    return QDBusUnixFileDescriptor();
}

QDBusUnixFileDescriptor DBusInterface::GetArtistArt(QString const& artist,
                                                    QString const& album,
                                                    QSize const& requestedSize)
{
    qDebug() << "Look up artist art for" << artist << "/" << album << "at size" << requestedSize;
    auto request = p->thumbnailer->get_artist_art(artist.toStdString(), album.toStdString(), requestedSize);
    queueRequest(new Handler(connection(), message(), p->check_thread_pool, p->create_thread_pool, std::move(request)));
    return QDBusUnixFileDescriptor();
}

QDBusUnixFileDescriptor DBusInterface::GetThumbnail(QString const& filename,
                                                    QDBusUnixFileDescriptor const& filename_fd,
                                                    QSize const& requestedSize)
{
    qDebug() << "Create thumbnail for" << filename << "at size" << requestedSize;

    std::unique_ptr<ThumbnailRequest> request;
    try
    {
        request = p->thumbnailer->get_thumbnail(filename.toStdString(), filename_fd.fileDescriptor(), requestedSize);
    }
    catch (unity::Exception const& e)
    {
        sendErrorReply(ART_ERROR, QString(e.to_string().c_str()));
        return QDBusUnixFileDescriptor();
    }
    catch (exception const& e)
    {
        sendErrorReply(ART_ERROR, e.what());
        return QDBusUnixFileDescriptor();
    }
    queueRequest(new Handler(connection(), message(), p->check_thread_pool, p->create_thread_pool, std::move(request)));
    return QDBusUnixFileDescriptor();
}

namespace
{

QDateTime to_date_time(chrono::steady_clock::time_point tp)
{
    // Conversion is somewhat awkward because steady_clock
    // uses time since boot as the epoch, and system_clock
    // is not guaranteed to use the same epoch as QDateTime.
    // (The C++ standard leaves the epoch time point undefined.)
    // We figure out the epoch for steady_clock and system
    // clock, so we we can convert to milliseconds since
    // the QDateTime epoch.
    static auto steady_now = chrono::steady_clock::now();
    static auto system_now = chrono::system_clock::now();
    static QDateTime qdt(QDate::currentDate(), QTime::currentTime());


    return QDateTime();
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

QVariantMap DBusInterface::Stats()
{
    auto st = p->thumbnailer->stats();
    
    QVariantMap m;
    m.insert("full_size_stats", to_variant_map(st.full_size_stats));
    m.insert("thumbnail_stats", to_variant_map(st.thumbnail_stats));
    m.insert("failure_stats", to_variant_map(st.failure_stats));
#if 0
    std::unique_ptr<ThumbnailRequest> request;
    try
    {
        request = p->thumbnailer->get_thumbnail(filename.toStdString(), filename_fd.fileDescriptor(), requestedSize);
    }
    catch (unity::Exception const& e)
    {
        sendErrorReply(ART_ERROR, QString(e.to_string().c_str()));
        return QDBusUnixFileDescriptor();
    }
    catch (exception const& e)
    {
        sendErrorReply(ART_ERROR, e.what());
        return QDBusUnixFileDescriptor();
    }
    queueRequest(new Handler(connection(), message(), p->check_thread_pool, p->create_thread_pool, std::move(request)));
    return QDBusUnixFileDescriptor();
#endif
    return m;
}

void DBusInterface::queueRequest(Handler* handler)
{
    p->requests.emplace(handler, std::unique_ptr<Handler>(handler));
    Q_EMIT endInactivity();
    connect(handler, &Handler::finished, this, &DBusInterface::requestFinished);
    setDelayedReply(true);
    handler->begin();
}

void DBusInterface::requestFinished()
{
    Handler* handler = static_cast<Handler*>(sender());
    try
    {
        auto& h = p->requests.at(handler);
        h.release();
        p->requests.erase(handler);
    }
    catch (std::out_of_range const& e)
    {
        qWarning() << "finished() called on unknown handler" << handler;
    }
    if (p->requests.empty())
    {
        Q_EMIT startInactivity();
    }
    // Queue deletion of handler when we re-enter the event loop.
    handler->deleteLater();
}
}
}
}
