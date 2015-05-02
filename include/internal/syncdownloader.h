/*
 * Copyright (C) 2015 Canonical Ltd
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
 * Authored by: Xavi Garcia <xavi.garcia.mena@canonical.com>
 */

#pragma once

#include <QMutex>
#include <QObject>
#include <QThread>
#include <QWaitCondition>

#include <atomic>
#include <memory>

namespace unity
{

namespace thumbnailer
{

namespace internal
{

class DownloadThread;
class ArtDownloader;

class SyncDownloader : public QObject
{
    Q_OBJECT
public:
    Q_DISABLE_COPY(SyncDownloader)

    SyncDownloader(std::shared_ptr<ArtDownloader> const &async_downloader);
    virtual ~SyncDownloader();

    QByteArray download_album(QString const& artist, QString const& album);
    QByteArray download_artist(QString const& artist, QString const& album);

Q_SIGNALS:
    void start_downloading_album(QString const&, QString const&);
    void start_downloading_artist(QString const&, QString const&);

private:
    std::shared_ptr<ArtDownloader> downloader_;
    QThread downloader_thread_;
    std::atomic<bool> downloading_;
    QMutex mutex_;
    QWaitCondition wait_downloader_;
    DownloadThread *downloader_worker_;
};

}  // namespace internal

}  // namespace thumbnailer

}  // namespace unity
