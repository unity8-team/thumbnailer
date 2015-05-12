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
 * Authored by: Xavi Garcia <xavi.garcia.mena@canonical.com>
 */

#include <internal/ubuntuserverdownloader.h>
#include <internal/artreply.h>

#include <QCoreApplication>
#include <QFile>
#include <QThread>

#include <atomic>
#include <cstdio>
#include <memory>
#include <string>

using namespace std;
using namespace unity::thumbnailer::internal;

class TestDownload : public QObject
{
    Q_OBJECT
public:
    enum DownloadType
    {
        UbuntuAlbum,
        UbuntuArtist
    };

    TestDownload(QObject *parent=nullptr) : QObject(parent), downloads_to_wait_(0)
    {
    }

    void start()
    {
        start(UbuntuArtist);
        start(UbuntuAlbum);
    }

    void start(DownloadType download_type)
    {
        downloads_to_wait_++;

        std::shared_ptr<ArtReply> reply;
        switch(download_type)
        {
        case UbuntuAlbum:
            reply = downloader_ubuntu_.download_album("u2", "joshua tree", 10000);
            if (reply)
            {
                map_active_downloads_[reply.get()] = UbuntuAlbum;
            }
            break;
        case UbuntuArtist:
            reply = downloader_ubuntu_.download_artist("u2", "joshua tree", 10000);
            if (reply)
            {
                map_active_downloads_[reply.get()] = UbuntuArtist;
            }
            break;
        default:
            abort();  // Impossible
            break;
        }
        if (reply)
        {
            connect(reply.get(), &ArtReply::finished, this, &TestDownload::download_finished);
        }
    }

public Q_SLOTS:
    void download_finished()
    {
        ArtReply * reply = static_cast<ArtReply*>(sender());
        if (reply->succeeded())
        {
            qDebug() << "FINISH: " << reply->url_string();
            std::map<ArtReply*, DownloadType>::iterator iter = map_active_downloads_.find(reply);
            if (iter != map_active_downloads_.end())
            {
                QString filename = QString("/tmp/test_thumnailer_%1%2")
                                                .arg(getTypeName((*iter).second))
                                                .arg(getTypeExtension((*iter).second));
                QFile file(filename);
                if (!file.open(QIODevice::WriteOnly))
                {
                    qCritical() << "Error opening the destination file: " << file.errorString();
                    return;
                }
                file.write(reply->data());
                file.close();
                qDebug() << "Wrote file: " << filename;

                map_active_downloads_.erase(iter);
            }
        }
        else
        {
            qDebug() << "FINISH ERROR: " << reply->url_string() << " " << reply->error_string();
        }
        downloads_to_wait_--;
        if (!downloads_to_wait_)
        {
            QCoreApplication::quit();
        }
    }

private:
    QString getTypeName(DownloadType type)
    {
        QString ret = "";
        switch(type)
        {
        case UbuntuAlbum:
            ret = "UbuntuAlbum";
            break;
        case UbuntuArtist:
            ret = "UbuntuArtist";
            break;
        default:
            abort();  // Impossible
            break;
        }
        return ret;
    }


    QString getTypeExtension(DownloadType type)
    {
        QString ret = "";
        switch(type)
        {
        case UbuntuAlbum:
            ret = ".jpg";
            break;
        case UbuntuArtist:
            ret = ".jpg";
            break;
        default:
            abort();  // Impossible
            break;
        }
        return ret;
    }

    std::map<ArtReply *, DownloadType> map_active_downloads_;
    UbuntuServerDownloader downloader_ubuntu_;
    int downloads_to_wait_;
};


int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    TestDownload test;
    test.start();
    return app.exec();
}

#include "downloaderstest.moc"
