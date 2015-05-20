
#include <internal/image.h>
#include <internal/raii.h>
#include "thumbnailerinterface.h"
#include "thumbnaileradmininterface.h"

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

using namespace std;
using namespace unity::thumbnailer::internal;

static const char BUS_NAME[] = "com.canonical.Thumbnailer";
static const char BUS_PATH[] = "/com/canonical/Thumbnailer";

static const char BUS_ADMIN_NAME[] = "com.canonical.ThumbnailerAdmin";
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
        fake_downloader_server_.setProcessChannelMode(QProcess::ForwardedErrorChannel);
        fake_downloader_server_.start("/usr/bin/python3", QStringList() << FAKE_DOWNLOADER_SERVER);
        ASSERT_TRUE(fake_downloader_server_.waitForStarted()) << "Failed to launch " << FAKE_DOWNLOADER_SERVER;
        ASSERT_GT(fake_downloader_server_.pid(), 0);
        ASSERT_TRUE(fake_downloader_server_.waitForReadyRead());
        QString port = QString::fromUtf8(fake_downloader_server_.readAllStandardOutput()).trimmed();

        QString apiroot = QString("http://127.0.0.1:%1").arg(port);
        setenv("THUMBNAILER_UBUNTU_APIROOT", apiroot.toUtf8().constData(), true);

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
            new ThumbnailerInterface(BUS_NAME, BUS_PATH,
                                     dbusTestRunner->sessionConnection()));

        admin_iface.reset(
            new ThumbnailerAdminInterface(BUS_ADMIN_NAME, BUS_ADMIN_PATH,
                                          dbusTestRunner->sessionConnection()));
        qDBusRegisterMetaType<unity::thumbnailer::service::AllStats>();
    }

    virtual void TearDown() override
    {
        iface.reset();
        dbusTestRunner.reset();

        unsetenv("THUMBNAILER_MAX_IDLE");
        unsetenv("XDG_CACHE_HOME");
        tempdir.reset();

        fake_downloader_server_.terminate();
        if (!fake_downloader_server_.waitForFinished())
        {
            qCritical() << "Failed to terminate fake server";
        }
        unsetenv("THUMBNAILER_UBUNTU_APIROOT");
    }

    unique_ptr<QTemporaryDir> tempdir;
    unique_ptr<QtDBusTest::DBusTestRunner> dbusTestRunner;
    unique_ptr<ThumbnailerInterface> iface;
    unique_ptr<ThumbnailerAdminInterface> admin_iface;
    QSharedPointer<QtDBusTest::QProcessDBusService> dbusService;
    QProcess fake_downloader_server_;
    QString apiroot_;
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
    QDBusReply<QVariantMap> reply = admin_iface->Stats();
    ASSERT_TRUE(reply.isValid());

#if 0
    QVariantMap m = reply.value();
    ASSERT_FALSE(m.empty());
    QMapIterator<QString, QVariant> i(m);
    while (i.hasNext())
    {
        i.next();
        cout << i.key().toStdString() << ": " << i.value().typeName() << endl;
        cerr << i.key().toStdString() << "Can convert to QDBusArgument: " << i.value().canConvert<QDBusArgument>() << endl;
        cerr << i.key().toStdString() << "Can convert to QVariant: " << i.value().canConvert<QVariant>() << endl;
    }
    QVariant v = qvariant_cast<QDBusArgument>(m.value("full_size_stats")).asVariant().toMap();
    cerr << "v: " << v.typeName() << endl;
    cerr << endl << endl;
    QVariantMap fullsize = qvariant_cast<QDBusVariant>(m.value("full_size_stats")).variant().toMap();
    EXPECT_FALSE(fullsize.empty());
#endif
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    setenv("TN_UTILDIR", TESTBINDIR "/../src/vs-thumb", true);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
