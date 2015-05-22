
#include <internal/image.h>
#include <internal/raii.h>
#include "thumbnailerinterface.h"
#include "admininterface.h"
#include "utils/artserver.h"

#include <boost/algorithm/string/predicate.hpp>
#include <gtest/gtest.h>
#include <libqtdbustest/DBusTestRunner.h>
#include <libqtdbustest/QProcessDBusService.h>
#include <QProcess>
#include <QSignalSpy>
#include <QTemporaryDir>

#include <testsetup.h>

#include <cstdlib>
#include <fcntl.h>
#include <memory>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <thread> // TODO: remove this

using namespace std;
using namespace unity::thumbnailer::internal;

static const char BUS_NAME[] = "com.canonical.Thumbnailer";

static const char BUS_THUMBNAILER_PATH[] = "/com/canonical/Thumbnailer";
static const char BUS_ADMIN_PATH[] = "/com/canonical/ThumbnailerAdmin";

template <typename T>
void assert_no_error(const QDBusReply<T>& reply)
{
    QString message;
    if (!reply.isValid())
    {
        auto error = reply.error();
        message = error.name() + ": " + error.message();
    }
    ASSERT_TRUE(reply.isValid()) << message.toUtf8().constData();
}

class DBusTest : public ::testing::Test
{
protected:
    DBusTest()
    {
    }
    virtual ~DBusTest()
    {
    }

    virtual void SetUp() override
    {
        // start fake server
        art_server_.reset(new ArtServer());

        // start dbus service
        tempdir.reset(new QTemporaryDir(TESTBINDIR "/dbus-test.XXXXXX"));
        setenv("XDG_CACHE_HOME", (tempdir->path() + "/cache").toUtf8().data(), true);

        dbusTestRunner.reset(new QtDBusTest::DBusTestRunner());

        // set 3 seconds as max idle time
        setenv("THUMBNAILER_MAX_IDLE", "1000", true);

        dbusService.reset(new QtDBusTest::QProcessDBusService(
            BUS_NAME, QDBusConnection::SessionBus, TESTBINDIR "/../src/service/thumbnailer-service", QStringList()));
        dbusTestRunner->registerService(dbusService);
        dbusTestRunner->startServices();

        iface.reset(
            new ThumbnailerInterface(BUS_NAME, BUS_THUMBNAILER_PATH,
                                     dbusTestRunner->sessionConnection()));

        admin_iface.reset(
            new ThumbnailerAdminInterface(BUS_NAME, BUS_ADMIN_PATH,
                                          dbusTestRunner->sessionConnection()));
        qDBusRegisterMetaType<unity::thumbnailer::service::AllStats>();
    }

    string temp_dir()
    {
        return tempdir->path().toStdString();
    }

    virtual void TearDown() override
    {
        iface.reset();
        admin_iface->Shutdown().waitForFinished();
        // For gcovr. We need to give the server some time to write its coverage data.
        this_thread::sleep_for(chrono::seconds(2));
        admin_iface.reset();
        dbusTestRunner.reset();

        art_server_.reset();

        unsetenv("THUMBNAILER_MAX_IDLE");
        unsetenv("XDG_CACHE_HOME");
        tempdir.reset();
    }

    unique_ptr<QTemporaryDir> tempdir;
    unique_ptr<QtDBusTest::DBusTestRunner> dbusTestRunner;
    unique_ptr<ThumbnailerInterface> iface;
    unique_ptr<ThumbnailerAdminInterface> admin_iface;
    QSharedPointer<QtDBusTest::QProcessDBusService> dbusService;
    std::unique_ptr<ArtServer> art_server_;
};

TEST_F(DBusTest, get_album_art)
{
    QDBusReply<QDBusUnixFileDescriptor> reply = iface->GetAlbumArt(
        "metallica", "load", QSize(24, 24));
    assert_no_error(reply);
    Image image(reply.value().fileDescriptor());
    EXPECT_EQ(24, image.width());
    EXPECT_EQ(24, image.width());
}

TEST_F(DBusTest, get_artist_art)
{
    QDBusReply<QDBusUnixFileDescriptor> reply = iface->GetArtistArt(
        "metallica", "load", QSize(24, 24));
    assert_no_error(reply);
    Image image(reply.value().fileDescriptor());
    EXPECT_EQ(24, image.width());
    EXPECT_EQ(24, image.width());
}

TEST_F(DBusTest, thumbnail_image)
{
    const char* filename = TESTDATADIR "/testimage.jpg";
    FdPtr fd(open(filename, O_RDONLY), do_close);
    ASSERT_GE(fd.get(), 0);

    QDBusReply<QDBusUnixFileDescriptor> reply = iface->GetThumbnail(
        filename, QDBusUnixFileDescriptor(fd.get()), QSize(256, 256));
    assert_no_error(reply);

    Image image(reply.value().fileDescriptor());
    EXPECT_EQ(256, image.width());
    EXPECT_EQ(160, image.height());
}

TEST_F(DBusTest, thumbnail_no_such_file)
{
    const char* no_such_file = TESTDATADIR "/no-such-file.jpg";
    const char* filename2 = TESTDATADIR "/testrotate.jpg";

    FdPtr fd(open(filename2, O_RDONLY), do_close);
    ASSERT_GE(fd.get(), 0);

    QDBusReply<QDBusUnixFileDescriptor> reply = iface->GetThumbnail(
        no_such_file, QDBusUnixFileDescriptor(fd.get()), QSize(256, 256));
    EXPECT_FALSE(reply.isValid());
    auto message = reply.error().message().toStdString();
    EXPECT_TRUE(boost::contains(message, " No such file or directory: ")) << message;
}

TEST_F(DBusTest, thumbnail_wrong_fd_fails)
{
    const char* filename1 = TESTDATADIR "/testimage.jpg";
    const char* filename2 = TESTDATADIR "/testrotate.jpg";

    FdPtr fd(open(filename2, O_RDONLY), do_close);
    ASSERT_GE(fd.get(), 0);

    QDBusReply<QDBusUnixFileDescriptor> reply = iface->GetThumbnail(
        filename1, QDBusUnixFileDescriptor(fd.get()), QSize(256, 256));
    EXPECT_FALSE(reply.isValid());
    auto message = reply.error().message().toStdString();
    EXPECT_TRUE(boost::contains(message, " file descriptor does not refer to file ")) << message;
}

TEST_F(DBusTest, test_inactivity_exit)
{
    // basic setup to the query
    const char* filename = TESTDATADIR "/testimage.jpg";
    FdPtr fd(open(filename, O_RDONLY), do_close);
    ASSERT_GE(fd.get(), 0);

    QSignalSpy spy_exit(&dbusService->underlyingProcess(),
                        static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished));

    // start a query
    QDBusReply<QDBusUnixFileDescriptor> reply = iface->GetThumbnail(
        filename, QDBusUnixFileDescriptor(fd.get()), QSize(256, 256));
    assert_no_error(reply);

    // wait for 5 seconds... (default)
    // Maximum inactivity should be less than that.
    spy_exit.wait();
    ASSERT_EQ(spy_exit.count(), 1);

    QList<QVariant> arguments = spy_exit.takeFirst();
    EXPECT_EQ(arguments.at(0).toInt(), 0);
}

TEST(DBusTestBadIdle, env_variable_bad_value)
{
    QTemporaryDir tempdir(TESTBINDIR "/dbus-test.XXXXXX");
    unique_ptr<QtDBusTest::DBusTestRunner> dbusTestRunner;
    unique_ptr<QDBusInterface> iface;
    QSharedPointer<QtDBusTest::QProcessDBusService> dbusService;

    setenv("XDG_CACHE_HOME", (tempdir.path() + "/cache").toUtf8().data(), true);

    dbusTestRunner.reset(new QtDBusTest::DBusTestRunner());

    setenv("THUMBNAILER_MAX_IDLE", "bad_value", true);

    dbusService.reset(new QtDBusTest::QProcessDBusService(
        BUS_NAME, QDBusConnection::SessionBus, TESTBINDIR "/../src/service/thumbnailer-service", QStringList()));

    dbusTestRunner->registerService(dbusService);
    dbusTestRunner->startServices();

    auto process = const_cast<QProcess*>(&dbusService->underlyingProcess());
    if (process->state() != QProcess::NotRunning)
    {
        EXPECT_EQ(process->waitForFinished(), true);
    }
    EXPECT_EQ(process->exitCode(), 1);

    unsetenv("THUMBNAILER_MAX_IDLE");
}

TEST_F(DBusTest, stats)
{
    using namespace unity::thumbnailer::service;

    QDBusReply<AllStats> reply = admin_iface->Stats();
    ASSERT_TRUE(reply.isValid()) << reply.error().message().toStdString();

    {
        CacheStats s = reply.value().full_size_stats;
        EXPECT_EQ(temp_dir() + "/cache/unity-thumbnailer/images", s.cache_path.toStdString());
        EXPECT_EQ(1, s.policy);
        EXPECT_EQ(0, s.size);
        EXPECT_EQ(0, s.size_in_bytes);
        EXPECT_NE(0, s.max_size_in_bytes);
        EXPECT_EQ(0, s.hits);
        EXPECT_EQ(0, s.misses);
        EXPECT_EQ(0, s.hits_since_last_miss);
        EXPECT_EQ(0, s.misses_since_last_hit);
        EXPECT_EQ(0, s.longest_hit_run);
        EXPECT_EQ(0, s.longest_miss_run);
        EXPECT_EQ(0, s.ttl_evictions);
        EXPECT_EQ(0, s.lru_evictions);
        EXPECT_EQ("Thu Jan 1 00:00:00 1970 GMT", s.most_recent_hit_time.toUTC().toString().toStdString());
        EXPECT_EQ("Thu Jan 1 00:00:00 1970 GMT", s.most_recent_miss_time.toUTC().toString().toStdString());
        EXPECT_EQ("Thu Jan 1 00:00:00 1970 GMT", s.longest_hit_run_time.toUTC().toString().toStdString());
        EXPECT_EQ("Thu Jan 1 00:00:00 1970 GMT", s.longest_miss_run_time.toUTC().toString().toStdString());
        auto list = s.histogram;
        for (auto c : list)
        {
            EXPECT_EQ(0, c);
        }
    }

    {
        CacheStats s = reply.value().thumbnail_stats;
        EXPECT_EQ(temp_dir() + "/cache/unity-thumbnailer/thumbnails", s.cache_path.toStdString());
        EXPECT_EQ(1, s.policy);
        EXPECT_EQ(0, s.size);
    }

    {
        CacheStats s = reply.value().failure_stats;
        EXPECT_EQ(temp_dir() + "/cache/unity-thumbnailer/failures", s.cache_path.toStdString());
        EXPECT_EQ(0, s.policy);
        EXPECT_EQ(0, s.size);
    }

    // Get a remote image from the cache, so the stats change.
    {
        QDBusReply<QDBusUnixFileDescriptor> reply = iface->GetAlbumArt(
            "metallica", "load", QSize(24, 24));
        assert_no_error(reply);
        Image image(reply.value().fileDescriptor());
        EXPECT_EQ(24, image.width());
        EXPECT_EQ(24, image.width());
    }

    reply = admin_iface->Stats();
    ASSERT_TRUE(reply.isValid()) << reply.error().message().toStdString();

    {
        CacheStats s = reply.value().full_size_stats;
        EXPECT_EQ(1, s.size);
        EXPECT_NE(0, s.size_in_bytes);
        EXPECT_EQ(0, s.hits);
        EXPECT_EQ(2, s.misses);
        EXPECT_EQ(0, s.hits_since_last_miss);
        EXPECT_EQ(2, s.misses_since_last_hit);
        EXPECT_EQ(0, s.longest_hit_run);
        EXPECT_EQ(2, s.longest_miss_run);
        EXPECT_EQ(0, s.ttl_evictions);
        EXPECT_EQ(0, s.lru_evictions);
        EXPECT_EQ("Thu Jan 1 00:00:00 1970 GMT", s.most_recent_hit_time.toUTC().toString().toStdString());
        EXPECT_NE("Thu Jan 1 00:00:00 1970 GMT", s.most_recent_miss_time.toUTC().toString().toStdString());
        EXPECT_EQ("Thu Jan 1 00:00:00 1970 GMT", s.longest_hit_run_time.toUTC().toString().toStdString());
        EXPECT_NE("Thu Jan 1 00:00:00 1970 GMT", s.longest_miss_run_time.toUTC().toString().toStdString());
        auto list = s.histogram;
        EXPECT_EQ(1, list[18]);
    }

    {
        CacheStats s = reply.value().thumbnail_stats;
        EXPECT_EQ(1, s.size);
        EXPECT_NE(0, s.size_in_bytes);
        EXPECT_EQ(0, s.hits);
        EXPECT_EQ(2, s.misses);
        EXPECT_EQ(0, s.hits_since_last_miss);
        EXPECT_EQ(2, s.misses_since_last_hit);
        EXPECT_EQ(0, s.longest_hit_run);
        EXPECT_EQ(2, s.longest_miss_run);
        EXPECT_EQ(0, s.ttl_evictions);
        EXPECT_EQ(0, s.lru_evictions);
        EXPECT_EQ("Thu Jan 1 00:00:00 1970 GMT", s.most_recent_hit_time.toUTC().toString().toStdString());
        EXPECT_NE("Thu Jan 1 00:00:00 1970 GMT", s.most_recent_miss_time.toUTC().toString().toStdString());
        EXPECT_EQ("Thu Jan 1 00:00:00 1970 GMT", s.longest_hit_run_time.toUTC().toString().toStdString());
        EXPECT_NE("Thu Jan 1 00:00:00 1970 GMT", s.longest_miss_run_time.toUTC().toString().toStdString());
    }

    // Get the same image again, so we get a hit.
    {
        QDBusReply<QDBusUnixFileDescriptor> reply = iface->GetAlbumArt(
            "metallica", "load", QSize(24, 24));
        assert_no_error(reply);
        Image image(reply.value().fileDescriptor());
        EXPECT_EQ(24, image.width());
        EXPECT_EQ(24, image.width());
    }

    reply = admin_iface->Stats();
    ASSERT_TRUE(reply.isValid()) << reply.error().message().toStdString();

    {
        CacheStats s = reply.value().thumbnail_stats;
        EXPECT_EQ(1, s.size);
        EXPECT_NE(0, s.size_in_bytes);
        EXPECT_EQ(1, s.hits);
        EXPECT_EQ(2, s.misses);
        EXPECT_EQ(1, s.hits_since_last_miss);
        EXPECT_EQ(0, s.misses_since_last_hit);
        EXPECT_EQ(1, s.longest_hit_run);
        EXPECT_EQ(2, s.longest_miss_run);
        EXPECT_EQ(0, s.ttl_evictions);
        EXPECT_EQ(0, s.lru_evictions);
        EXPECT_NE("Thu Jan 1 00:00:00 1970 GMT", s.most_recent_hit_time.toUTC().toString().toStdString());
        EXPECT_NE("Thu Jan 1 00:00:00 1970 GMT", s.most_recent_miss_time.toUTC().toString().toStdString());
        EXPECT_NE("Thu Jan 1 00:00:00 1970 GMT", s.longest_hit_run_time.toUTC().toString().toStdString());
        EXPECT_NE("Thu Jan 1 00:00:00 1970 GMT", s.longest_miss_run_time.toUTC().toString().toStdString());
    }

    // Get a non-existent remote image from the cache, so the failure stats change.
    {
        QDBusReply<QDBusUnixFileDescriptor> reply = iface->GetAlbumArt(
            "no_such_artist", "no_such_album", QSize(24, 24));
    }

    reply = admin_iface->Stats();
    ASSERT_TRUE(reply.isValid());

    {
        CacheStats s = reply.value().failure_stats;
        EXPECT_EQ(1, s.size);
        EXPECT_EQ(0, s.hits);
        EXPECT_EQ(4, s.misses);
    }

    // Get the same non-existent remote image again, so we get a hit.
    {
        QDBusReply<QDBusUnixFileDescriptor> reply = iface->GetAlbumArt(
            "no_such_artist", "no_such_album", QSize(24, 24));
    }

    reply = admin_iface->Stats();
    ASSERT_TRUE(reply.isValid()) << reply.error().message().toStdString();

    {
        CacheStats s = reply.value().failure_stats;
        EXPECT_EQ(1, s.size);
        EXPECT_EQ(2, s.hits);
        EXPECT_EQ(4, s.misses);
    }
}

Q_DECLARE_METATYPE(QProcess::ExitStatus)  // Avoid noise from signal spy.

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    qRegisterMetaType<QProcess::ExitStatus>("QProcess::ExitStatus");  // Avoid noise from signal spy.

    setenv("TN_UTILDIR", TESTBINDIR "/../src/vs-thumb", true);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
