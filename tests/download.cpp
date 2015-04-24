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
#include <internal/lastfmdownloader.h>
#include <internal/artreply.h>

#include <gtest/gtest.h>

#include <QtTest/QtTest>
#include <QtTest/QSignalSpy>
#include <QVector>

#include <core/posix/exec.h>

#include <chrono>
#include <thread>

using namespace unity::thumbnailer::internal;
namespace posix = core::posix;

// Thread to download a file and check its content
// The fake server generates specific file content when the given artist is "test_threads"

// The content coming from the fake server is: TEST_THREADS_TEST_ + the give download_id.
// Example: download_id = "TEST_1" --> content is: "TEST_THREADS_TEST_TEST_1"
class UbuntuServerWorkerThread : public QThread
{
    Q_OBJECT
public:
    UbuntuServerWorkerThread(QString download_id, QObject* parent = nullptr)
        : QThread(parent)
        , download_id_(download_id)
    {
    }

private:
    void run() Q_DECL_OVERRIDE
    {
        UbuntuServerDownloader downloader;
        auto reply = downloader.download_album("test_threads", download_id_);
        ASSERT_NE(reply, nullptr);

        QSignalSpy spy(reply, SIGNAL(finished()));

        // check the returned url
        QString url_to_check =
            QString(
                "/musicproxy/v1/album-art?artist=test_threads&album=%1&size=350&key=0f450aa882a6125ebcbfb3d7f7aa25bc")
                .arg(download_id_);
        ASSERT_TRUE(reply->url_string().endsWith(url_to_check) == true);

        // we set a timeout of 5 seconds waiting for the signal to be emitted,
        // which should never be reached
        spy.wait(5000);

        // check that we've got exactly one signal
        ASSERT_EQ(spy.count(), 1);

        // Finally check the content of the file downloaded
        EXPECT_EQ(QString(reply->data()), QString("TEST_THREADS_TEST_%1").arg(download_id_));
        EXPECT_EQ(reply->succeded(), true);
        EXPECT_EQ(reply->not_found_error(), false);
        EXPECT_EQ(reply->is_running(), false);
        reply->deleteLater();
    }

private:
    QString download_id_;
};

// Thread to download a file and check its content
// The fake server generates specific file content when the given artist is "test"

// The content coming from the fake server is: TEST_THREADS_TEST_ + "test_thread" + the give download_id.
// Example: download_id = "TEST_1" --> content is: "TEST_THREADS_TEST_test_thread_TEST_1"
class LastFMWorkerThread : public QThread
{
    Q_OBJECT
public:
    LastFMWorkerThread(QString download_id, QObject* parent = nullptr)
        : QThread(parent)
        , download_id_(download_id)
    {
    }

private:
    void run() Q_DECL_OVERRIDE
    {
        LastFMDownloader downloader;
        auto reply = downloader.download_album("test", QString("thread_%1").arg(download_id_));
        ASSERT_NE(reply, nullptr);

        QSignalSpy spy(reply, SIGNAL(finished()));

        QString url_to_check = QString("/1.0/album/test/thread_%1/info.xml").arg(download_id_);
        // check the returned url
        EXPECT_EQ(reply->url_string().endsWith(url_to_check), true);

        // we set a timeout of 5 seconds waiting for the signal to be emitted,
        // which should never be reached
        spy.wait(5000);

        // check that we've got exactly one signal
        ASSERT_EQ(spy.count(), 1);

        EXPECT_EQ(reply->succeded(), true);
        EXPECT_EQ(reply->not_found_error(), false);
        EXPECT_EQ(reply->is_running(), false);
        // Finally check the content of the file downloaded
        EXPECT_EQ(QString(reply->data()), QString("TEST_THREADS_TEST_test_thread_%1").arg(download_id_));
        reply->deleteLater();
    }

private:
    QString download_id_;
};

class TestDownloaderServer : public ::testing::Test
{
protected:
    void SetUp() override
    {
        fake_downloader_server_ = posix::exec(FAKE_DOWNLOADER_SERVER,
                {server_argv_.toStdString(), QString("%1").arg(number_of_errors_before_ok_).toStdString()},
                {},
                posix::StandardStream::stdout);

        ASSERT_GT(fake_downloader_server_.pid(), 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        std::string port;
        fake_downloader_server_.cout() >> port;

        apiroot_ = "http://127.0.0.1:" + QString::fromStdString(port);
        setenv("THUMBNAILER_LASTFM_APIROOT", apiroot_.toStdString().c_str(), true);
        setenv("THUMBNAILER_UBUNTU_APIROOT", apiroot_.toStdString().c_str(), true);
    }

    void TearDown() override
    {
        unsetenv("THUMBNAILER_LASTFM_APIROOT");
        unsetenv("THUMBNAILER_UBUNTU_APIROOT");
    }

    posix::ChildProcess fake_downloader_server_ = posix::ChildProcess::invalid();
    QString apiroot_;
    QString server_argv_;
    int number_of_errors_before_ok_;

};

TEST_F(TestDownloaderServer, test_ok_album)
{
    UbuntuServerDownloader downloader;

    auto reply = downloader.download_album("sia", "fear");
    ASSERT_NE(reply, nullptr);

    EXPECT_EQ(
        reply->url_string().endsWith("/musicproxy/v1/album-art?artist=sia&album=fear&size=350&key=0f450aa882a6125ebcbfb3d7f7aa25bc"),
        true);

    QSignalSpy spy(reply, SIGNAL(finished()));

    // we set a timeout of 5 seconds waiting for the signal to be emitted,
    // which should never be reached
    spy.wait(5000);

    // check that we've got exactly one signal
    ASSERT_EQ(spy.count(), 1);

    EXPECT_EQ(reply->succeded(), true);
    EXPECT_EQ(reply->not_found_error(), false);
    EXPECT_EQ(reply->is_running(), false);
    // Finally check the content of the file downloaded
    EXPECT_EQ(QString(reply->data()), QString("SIA_FEAR_TEST_STRING_IMAGE"));

    reply->deleteLater();
}

TEST_F(TestDownloaderServer, test_ok_artist)
{
    UbuntuServerDownloader downloader;

    auto reply = downloader.download_artist("sia", "fear");
    ASSERT_NE(reply, nullptr);

    EXPECT_EQ(
            reply->url_string().endsWith("/musicproxy/v1/artist-art?artist=sia&album=fear&size=300&key=0f450aa882a6125ebcbfb3d7f7aa25bc"),
        true);

    QSignalSpy spy(reply, SIGNAL(finished()));
    // we set a timeout of 5 seconds waiting for the signal to be emitted,
    // which should never be reached
    spy.wait(5000);

    // check that we've got exactly one signal
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(reply->succeded(), true);
    EXPECT_EQ(reply->not_found_error(), false);
    EXPECT_EQ(reply->is_running(), false);
    EXPECT_EQ(QString(reply->data()), QString("SIA_FEAR_TEST_STRING_IMAGE"));

    reply->deleteLater();
}

TEST_F(TestDownloaderServer, test_not_found)
{
    UbuntuServerDownloader downloader;

    auto reply = downloader.download_album("test", "test");
    ASSERT_NE(reply, nullptr);

    EXPECT_EQ(
        reply->url_string().endsWith("/musicproxy/v1/album-art?artist=test&album=test&size=350&key=0f450aa882a6125ebcbfb3d7f7aa25bc"),
        true);

    QSignalSpy spy(reply, SIGNAL(finished()));
    // we set a timeout of 5 seconds waiting for the signal to be emitted,
    // which should never be reached
    spy.wait(5000);

    // check that we've got exactly one signal
    ASSERT_EQ(spy.count(), 1);

    EXPECT_EQ(reply->succeded(), false);
    EXPECT_EQ(reply->not_found_error(), false);
    EXPECT_EQ(reply->is_running(), false);
    EXPECT_EQ(reply->error_string().endsWith(
                  "/musicproxy/v1/album-art?artist=test&album=test&size=350&key=0f450aa882a6125ebcbfb3d7f7aa25bc - "
                  "server replied: Internal Server Error"),
              true);

    reply->deleteLater();
}

TEST_F(TestDownloaderServer, test_threads)
{
    QVector<UbuntuServerWorkerThread*> threads;

    int NUM_THREADS = 100;
    for (auto i = 0; i < NUM_THREADS; ++i)
    {
        QString download_id = QString("TEST_%1").arg(i);
        threads.push_back(new UbuntuServerWorkerThread(download_id));
    }

    for (auto i = 0; i < NUM_THREADS; ++i)
    {
        threads[i]->start();
    }

    for (auto i = 0; i < NUM_THREADS; ++i)
    {
        threads[i]->wait();
    }

    for (auto th : threads)
    {
        delete th;
    }
}

TEST_F(TestDownloaderServer, lastfm_download_ok)
{
    LastFMDownloader downloader;

    auto reply = downloader.download_album("sia", "fear");
    ASSERT_NE(reply, nullptr);

    QSignalSpy spy(reply, SIGNAL(finished()));

    EXPECT_EQ(reply->url_string(), apiroot_ + "/1.0/album/sia/fear/info.xml");

    // we set a timeout of 5 seconds waiting for the signal to be emitted,
    // which should never be reached
    spy.wait(5000);

    // check that we've got exactly one signal
    ASSERT_EQ(spy.count(), 1);

    EXPECT_EQ(reply->succeded(), true);
    EXPECT_EQ(reply->not_found_error(), false);
    EXPECT_EQ(reply->is_running(), false);
    // Finally check the content of the file downloaded
    EXPECT_EQ(QString(reply->data()), QString("SIA_FEAR_TEST_STRING_IMAGE"));

    reply->deleteLater();
}

TEST_F(TestDownloaderServer, lastfm_xml_parsing_errors)
{
    LastFMDownloader downloader;

    auto reply = downloader.download_album("xml", "errors");
    ASSERT_NE(reply, nullptr);

    QSignalSpy spy(reply, SIGNAL(finished()));

    EXPECT_EQ(reply->url_string(), apiroot_ + "/1.0/album/xml/errors/info.xml");

    // we set a timeout of 5 seconds waiting for the signal to be emitted,
    // which should never be reached
    spy.wait(5000);

    // check that we've got exactly one signal
    ASSERT_EQ(spy.count(), 1);

    EXPECT_EQ(reply->succeded(), false);
    EXPECT_EQ(reply->not_found_error(), false);
    EXPECT_EQ(reply->is_running(), false);
    // Finally check the content of the error message
    EXPECT_EQ(reply->error_string(),
              QString("LastFMDownloader::parse_xml() XML ERROR: Expected '?', '!', or '[a-zA-Z]', but got '/'."));

    reply->deleteLater();
}

TEST_F(TestDownloaderServer, lastfm_xml_image_not_found)
{
    LastFMDownloader downloader;

    auto reply = downloader.download_album("no", "cover");
    ASSERT_NE(reply, nullptr);

    QSignalSpy spy(reply, SIGNAL(finished()));

    EXPECT_EQ(reply->url_string(), apiroot_ + "/1.0/album/no/cover/info.xml");

    // we set a timeout of 5 seconds waiting for the signal to be emitted,
    // which should never be reached
    spy.wait(5000);

    // check that we've got exactly one signal
    ASSERT_EQ(spy.count(), 1);

    EXPECT_EQ(reply->succeded(), false);
    EXPECT_EQ(reply->not_found_error(), false);
    EXPECT_EQ(reply->is_running(), false);
    // Finally check the content of the error message
    EXPECT_EQ(reply->error_string(), QString("LastFMDownloader::parse_xml() Image url not found"));

    reply->deleteLater();
}

TEST_F(TestDownloaderServer, lastfm_test_threads)
{
    QVector<LastFMWorkerThread*> threads;

    int NUM_THREADS = 100;
    for (auto i = 0; i <= NUM_THREADS; ++i)
    {
        // we set the id to modulus 5 + 1 as the query xml that
        // we have in the fake server are valid only from 1 to 5
        QString download_id = QString("%1").arg((i % 5) + 1);
        threads.push_back(new LastFMWorkerThread(download_id));
    }

    for (auto i = 0; i < NUM_THREADS; ++i)
    {
        threads[i]->start();
    }

    for (auto i = 0; i < NUM_THREADS; ++i)
    {
        threads[i]->wait();
    }

    for (auto th : threads)
    {
        delete th;
    }
}

int main(int argc, char** argv)
{
    QCoreApplication qt_app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

// QTEST_MAIN(TestDownloader)
#include "download.moc"
