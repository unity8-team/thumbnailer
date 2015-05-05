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

#include <internal/syncdownloader.h>
#include <internal/artdownloader.h>
#include <internal/artreply.h>

#include <cassert>

#include <QEventLoop>

namespace unity
{

namespace thumbnailer
{

namespace internal
{

class DownloadThread : public QObject
{
    Q_OBJECT
public:
    DownloadThread(std::shared_ptr<ArtDownloader> const& downloader, QWaitCondition & wait_cond, QObject *parent = nullptr)
        :QObject(parent), downloader_(downloader), wait_cond_(wait_cond)
    {
    }

    QByteArray const& downloaded_data()
    {
        return downloaded_data_;
    }

public Q_SLOTS:

    void download_album(QString const& artist, QString const& album)
    {
        auto reply = downloader_->download_album(artist, album);
        connect(reply.get(), &ArtReply::finished, this, &DownloadThread::download_finished);
        QEventLoop loop;
        QObject::connect(reply.get(), SIGNAL(finished()), &loop, SLOT(quit()));
        loop.exec();
    }

    void download_artist(QString const& artist, QString const& album)
    {
        auto reply = downloader_->download_artist(artist, album);
        connect(reply.get(), &ArtReply::finished, this, &DownloadThread::download_finished);
        QEventLoop loop;
        QObject::connect(reply.get(), SIGNAL(finished()), &loop, SLOT(quit()));
        loop.exec();
    }

protected Q_SLOTS:
    void download_finished()
    {
        ArtReply *reply = static_cast<ArtReply*>(sender());
        assert(reply);
        if (reply->succeeded())
        {
            downloaded_data_ = reply->data();;
        }
        else
        {
            downloaded_data_ = QByteArray();
        }
        wait_cond_.wakeAll();
    }

private:
    std::shared_ptr<ArtDownloader> downloader_;
    QByteArray downloaded_data_;
    QWaitCondition &wait_cond_;
};


SyncDownloader::SyncDownloader(std::shared_ptr<ArtDownloader> const &async_downloader)
    : downloader_(async_downloader),
      downloading_(false),
      downloader_worker_(new DownloadThread(async_downloader, wait_downloader_))
{
    downloader_worker_->moveToThread(&downloader_thread_);
    async_downloader->moveToThread(&downloader_thread_);
    connect(&downloader_thread_, &QThread::finished, downloader_worker_, &QObject::deleteLater);
    connect(this, &SyncDownloader::start_downloading_album, downloader_worker_, &DownloadThread::download_album);
    connect(this, &SyncDownloader::start_downloading_artist, downloader_worker_, &DownloadThread::download_artist);
    downloader_thread_.start();
}

SyncDownloader::~SyncDownloader()
{
    downloader_thread_.quit();
    downloader_thread_.wait();
}

QByteArray SyncDownloader::download_album(QString const& artist, QString const& album)
{
    QMutexLocker locker(&mutex_);
    Q_EMIT start_downloading_album(artist, album);
    wait_downloader_.wait(&mutex_);
    return downloader_worker_->downloaded_data();
}

QByteArray SyncDownloader::download_artist(QString const& artist, QString const& album)
{
    QMutexLocker locker(&mutex_);
    Q_EMIT start_downloading_artist(artist, album);
    wait_downloader_.wait(&mutex_);
    return downloader_worker_->downloaded_data();
}

}  // namespace internal

}  // namespace thumbnailer

}  // namespace unity

#include "syncdownloader.moc"
