/*
 * Copyright (C) 2014 Canonical Ltd.
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

#include <gtest/gtest.h>

#include <QProcess>
#include <QtTest/QtTest>
#include <QtTest/QSignalSpy>
#include <QVector>

#include <chrono>
#include <thread>
#include <vector>

using namespace unity::thumbnailer::internal;

class TestDownloaderServer : public ::testing::Test
{
protected:
    void SetUp() override
    {
        fake_downloader_server_.setProcessChannelMode(QProcess::ForwardedErrorChannel);
        fake_downloader_server_.start("/usr/bin/python3", QStringList() << FAKE_DOWNLOADER_SERVER);
        ASSERT_TRUE(fake_downloader_server_.waitForStarted()) << "Failed to launch " << FAKE_DOWNLOADER_SERVER;
        ASSERT_GT(fake_downloader_server_.pid(), 0);
        ASSERT_TRUE(fake_downloader_server_.waitForReadyRead());
        QString port = QString::fromUtf8(fake_downloader_server_.readAllStandardOutput()).trimmed();

        apiroot_ = QString("http://127.0.0.1:%1").arg(port);
        setenv("THUMBNAILER_UBUNTU_APIROOT", apiroot_.toUtf8().constData(), true);
    }

    void TearDown() override
    {
        fake_downloader_server_.terminate();
        if (!fake_downloader_server_.waitForFinished())
        {
            qCritical() << "Failed to terminate fake server";
        }
        unsetenv("THUMBNAILER_UBUNTU_APIROOT");
    }

    QProcess fake_downloader_server_;
    QString apiroot_;
    QString server_argv_;
    int number_of_errors_before_ok_;

};

TEST_F(TestDownloaderServer, test_download_album_url)
{
    UbuntuServerDownloader downloader;

    auto reply = downloader.download_album("sia", "fear");
    ASSERT_NE(reply, nullptr);

    QUrl check_url(reply->url_string());
    QUrlQuery url_query(check_url.query());
    EXPECT_EQ(url_query.queryItemValue("artist"), "sia");
    EXPECT_EQ(url_query.queryItemValue("album"), "fear");
    EXPECT_EQ(check_url.path(), "/musicproxy/v1/album-art");
    qDebug() << check_url.toString();
    EXPECT_TRUE(check_url.toString().startsWith(apiroot_));
}

TEST_F(TestDownloaderServer, test_download_artist_url)
{
    UbuntuServerDownloader downloader;

    auto reply = downloader.download_artist("sia", "fear");
    ASSERT_NE(reply, nullptr);

    QUrl check_url(reply->url_string());
    QUrlQuery url_query(check_url.query());
    EXPECT_EQ(url_query.queryItemValue("artist"), "sia");
    EXPECT_EQ(url_query.queryItemValue("album"), "fear");
    EXPECT_EQ(check_url.path(), "/musicproxy/v1/artist-art");
    EXPECT_TRUE(check_url.toString().startsWith(apiroot_));
}

TEST_F(TestDownloaderServer, test_ok_album)
{
    UbuntuServerDownloader downloader;

    auto reply = downloader.download_album("sia", "fear");
    ASSERT_NE(reply, nullptr);

    QSignalSpy spy(reply.get(), &ArtReply::finished);

    // we set a timeout of 5 seconds waiting for the signal to be emitted,
    // which should never be reached
    ASSERT_TRUE(spy.wait(5000));

    // check that we've got exactly one signal
    ASSERT_EQ(spy.count(), 1);

    EXPECT_EQ(reply->succeeded(), true);
    EXPECT_EQ(reply->not_found_error(), false);
    EXPECT_EQ(reply->is_running(), false);
    EXPECT_EQ(reply->network_error(), false);
    // Finally check the content of the file downloaded
    EXPECT_EQ(QString(reply->data()), QString("SIA_FEAR_TEST_STRING_IMAGE_ALBUM"));
}

TEST_F(TestDownloaderServer, test_ok_artist)
{
    UbuntuServerDownloader downloader;

    auto reply = downloader.download_artist("sia", "fear");
    ASSERT_NE(reply, nullptr);

    QSignalSpy spy(reply.get(), &ArtReply::finished);
    // we set a timeout of 5 seconds waiting for the signal to be emitted,
    // which should never be reached
    ASSERT_TRUE(spy.wait(5000));

    // check that we've got exactly one signal
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(reply->succeeded(), true);
    EXPECT_EQ(reply->not_found_error(), false);
    EXPECT_EQ(reply->is_running(), false);
    EXPECT_EQ(reply->network_error(), false);
    EXPECT_EQ(QString(reply->data()), QString("SIA_FEAR_TEST_STRING_IMAGE"));
}

TEST_F(TestDownloaderServer, test_not_found)
{
    UbuntuServerDownloader downloader;

    auto reply = downloader.download_album("test", "test");
    ASSERT_NE(reply, nullptr);

    QSignalSpy spy(reply.get(), &ArtReply::finished);
    // we set a timeout of 5 seconds waiting for the signal to be emitted,
    // which should never be reached
    ASSERT_TRUE(spy.wait(5000));

    // check that we've got exactly one signal
    ASSERT_EQ(spy.count(), 1);

    EXPECT_EQ(reply->succeeded(), false);
    EXPECT_EQ(reply->not_found_error(), true);
    EXPECT_EQ(reply->is_running(), false);
    EXPECT_EQ(reply->network_error(), false);
    EXPECT_TRUE(reply->error_string().endsWith("server replied: Not Found"));
}

TEST_F(TestDownloaderServer, test_multiple_downloads)
{
    UbuntuServerDownloader downloader;

    std::vector<std::pair<std::shared_ptr<ArtReply>, std::shared_ptr<QSignalSpy>>> replies;

    int NUM_DOWNLOADS = 100;
    for (auto i = 0; i < NUM_DOWNLOADS; ++i)
    {
        QString download_id = QString("TEST_%1").arg(i);
        auto reply = downloader.download_album("test_threads", download_id);
        ASSERT_NE(reply, nullptr);
        std::shared_ptr<QSignalSpy> spy(new QSignalSpy(reply.get(), &ArtReply::finished));
        replies.push_back(std::make_pair(reply, spy));
    }

    for (auto i = 0; i < NUM_DOWNLOADS; ++i)
    {
        if (!replies[i].second->count())
        {
            // if it was not called yet, wait for it
            ASSERT_TRUE(replies[i].second->wait());
        }
        ASSERT_EQ(replies[i].second->count(), 1);
        EXPECT_EQ(replies[i].first->succeeded(), true);
        EXPECT_EQ(replies[i].first->not_found_error(), false);
        EXPECT_EQ(replies[i].first->is_running(), false);
        EXPECT_EQ(replies[i].first->network_error(), false);
        // Finally check the content of the file downloaded
        EXPECT_EQ(QString(replies[i].first->data()), QString("TEST_THREADS_TEST_TEST_%1").arg(i));
    }
}

TEST_F(TestDownloaderServer, test_connection_error)
{
    UbuntuServerDownloader downloader;

    auto network_manager = downloader.network_manager();

    // disable connection before executing any query
    network_manager->setNetworkAccessible(QNetworkAccessManager::NotAccessible);

    auto reply = downloader.download_artist("sia", "fear");
    ASSERT_NE(reply, nullptr);

    QSignalSpy spy(reply.get(), &ArtReply::finished);
    // we set a timeout of 5 seconds waiting for the signal to be emitted,
    // which should never be reached
    ASSERT_TRUE(spy.wait(5000));

    // check that we've got exactly one signal
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(reply->succeeded(), false);
    EXPECT_EQ(reply->not_found_error(), false);
    EXPECT_EQ(reply->is_running(), false);
    EXPECT_EQ(reply->network_error(), true);
}

int main(int argc, char** argv)
{
    QCoreApplication qt_app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
