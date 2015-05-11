
#include <internal/raii.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <cstdlib>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <boost/algorithm/string/predicate.hpp>
#include <gtest/gtest.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wcast-qual"
#include <gdk-pixbuf/gdk-pixbuf.h>
#pragma GCC diagnostic pop
#include <libqtdbustest/DBusTestRunner.h>
#include <libqtdbustest/QProcessDBusService.h>
#include <unity/util/ResourcePtr.h>

#include <QSignalSpy>
#include <QProcess>
#include <QTemporaryDir>

#include <testsetup.h>

using namespace unity::thumbnailer::internal;

static const char BUS_NAME[] = "com.canonical.Thumbnailer";
static const char BUS_PATH[] = "/com/canonical/Thumbnailer";
static const char THUMBNAILER_IFACE[] = "com.canonical.Thumbnailer";

typedef std::unique_ptr<GdkPixbuf, decltype(&g_object_unref)> PixbufPtr;

template <typename T>
void assert_no_error(const QDBusReply<T> &reply) {
    QString message;
    if (!reply.isValid()) {
        auto error = reply.error();
        message = error.name() + ": " + error.message();
    }
    ASSERT_TRUE(reply.isValid()) << message.toUtf8().constData();
}

PixbufPtr read_image(const QDBusUnixFileDescriptor &unix_fd) {
    auto close_loader = [](GdkPixbufLoader *loader) {
        if (loader) {
            gdk_pixbuf_loader_close(loader, nullptr);
            g_object_unref(loader);
        }
    };
    std::unique_ptr<GdkPixbufLoader, decltype(close_loader)> loader(
        gdk_pixbuf_loader_new(), close_loader);
    if (!loader) {
        throw std::runtime_error("read_image: could not create pixbuf loader");
    }

    unsigned char buffer[4096];
    ssize_t n_read;
    GError *error = nullptr;
    while ((n_read = read(unix_fd.fileDescriptor(), buffer, sizeof(buffer))) > 0) {
        if (!gdk_pixbuf_loader_write(loader.get(), buffer, n_read, &error)) {
            std::string message("read_image: error writing to loader: ");
            message += error->message;
            g_error_free(error);
            throw std::runtime_error(message);
        }
    }
    if (n_read < 0) {
        throw std::runtime_error("read_image: error reading from file descriptor");
    }
    if (!gdk_pixbuf_loader_close(loader.get(), &error)) {
        std::string message("read_image: error closing loader: ");
        message += error->message;
        g_error_free(error);
        throw std::runtime_error(message);
    }
    GdkPixbuf *pixbuf = gdk_pixbuf_loader_get_pixbuf(loader.get());
    if (!pixbuf) {
        throw std::runtime_error("read_image: could not create pixbuf");
    }
    return PixbufPtr(static_cast<GdkPixbuf*>(g_object_ref(pixbuf)),
                     g_object_unref);
}

class DBusTest : public ::testing::Test {
protected:
    DBusTest() {}
    virtual ~DBusTest() {}

    virtual void SetUp() override {
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
                BUS_NAME, QDBusConnection::SessionBus,
                TESTBINDIR "/../src/service/thumbnailer-service",
                QStringList()));
        dbusTestRunner->registerService(dbusService);
        dbusTestRunner->startServices();

        iface.reset(
            new QDBusInterface(BUS_NAME, BUS_PATH, THUMBNAILER_IFACE,
                               dbusTestRunner->sessionConnection()));
    }

    virtual void TearDown() override {
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

    std::unique_ptr<QTemporaryDir> tempdir;
    std::unique_ptr<QtDBusTest::DBusTestRunner> dbusTestRunner;
    std::unique_ptr<QDBusInterface> iface;
    QSharedPointer<QtDBusTest::QProcessDBusService> dbusService;
    QProcess fake_downloader_server_;
    QString apiroot_;
};

TEST_F(DBusTest, get_album_art) {
    QDBusReply<QDBusUnixFileDescriptor> reply = iface->call(
        "GetAlbumArt", "metallica", "load", QSize(24, 24));
    assert_no_error(reply);
    auto pixbuf = read_image(reply.value());
    EXPECT_EQ(24, gdk_pixbuf_get_width(pixbuf.get()));
    EXPECT_EQ(24, gdk_pixbuf_get_height(pixbuf.get()));
}

TEST_F(DBusTest, get_artist_art) {
    QDBusReply<QDBusUnixFileDescriptor> reply = iface->call(
        "GetArtistArt", "metallica", "load", QSize(24, 24));
    assert_no_error(reply);
    auto pixbuf = read_image(reply.value());
    EXPECT_EQ(24, gdk_pixbuf_get_width(pixbuf.get()));
    EXPECT_EQ(24, gdk_pixbuf_get_height(pixbuf.get()));
}

TEST_F(DBusTest, thumbnail_image) {
    const char *filename = TESTDATADIR "/testimage.jpg";
    FdPtr fd(open(filename, O_RDONLY), do_close);
    ASSERT_GE(fd.get(), 0);

    QDBusReply<QDBusUnixFileDescriptor> reply = iface->call(
        "GetThumbnail", filename,
        QVariant::fromValue(QDBusUnixFileDescriptor(fd.get())),
        QSize(256, 256));
    assert_no_error(reply);

    auto pixbuf = read_image(reply.value());
    EXPECT_EQ(256, gdk_pixbuf_get_width(pixbuf.get()));
    EXPECT_EQ(160, gdk_pixbuf_get_height(pixbuf.get()));
}

TEST_F(DBusTest, thumbnail_no_such_file) {
    const char *no_such_file = TESTDATADIR "/no-such-file.jpg";
    const char *filename2 = TESTDATADIR "/testrotate.jpg";

    FdPtr fd(open(filename2, O_RDONLY), do_close);
    ASSERT_GE(fd.get(), 0);

    QDBusReply<QDBusUnixFileDescriptor> reply = iface->call(
        "GetThumbnail", no_such_file,
        QVariant::fromValue(QDBusUnixFileDescriptor(fd.get())),
        QSize(256, 256));
    EXPECT_FALSE(reply.isValid());
    auto message = reply.error().message().toStdString();
    EXPECT_TRUE(boost::starts_with(message, "ThumbnailHandler::create(): Could not stat ")) << message;
}

TEST_F(DBusTest, thumbnail_wrong_fd_fails) {
    const char *filename1 = TESTDATADIR "/testimage.jpg";
    const char *filename2 = TESTDATADIR "/testrotate.jpg";

    FdPtr fd(open(filename2, O_RDONLY), do_close);
    ASSERT_GE(fd.get(), 0);

    QDBusReply<QDBusUnixFileDescriptor> reply = iface->call(
        "GetThumbnail", filename1,
        QVariant::fromValue(QDBusUnixFileDescriptor(fd.get())),
        QSize(256, 256));
    EXPECT_FALSE(reply.isValid());
    auto message = reply.error().message().toStdString();
    EXPECT_TRUE(boost::ends_with(message, " refers to a different file than the file descriptor"));
}

TEST_F(DBusTest, test_inactivity_exit) {
    // basic setup to the query
    const char *filename = TESTDATADIR "/testimage.jpg";
    FdPtr fd(open(filename, O_RDONLY), do_close);
    ASSERT_GE(fd.get(), 0);

    QSignalSpy spy_exit(&dbusService->underlyingProcess(), static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished));

    // start a query
    QDBusReply<QDBusUnixFileDescriptor> reply = iface->call(
        "GetThumbnail", filename,
        QVariant::fromValue(QDBusUnixFileDescriptor(fd.get())),
        QSize(256, 256));
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
    std::unique_ptr<QtDBusTest::DBusTestRunner> dbusTestRunner;
    std::unique_ptr<QDBusInterface> iface;
    QSharedPointer<QtDBusTest::QProcessDBusService> dbusService;

    setenv("XDG_CACHE_HOME", (tempdir.path() + "/cache").toUtf8().data(), true);

    dbusTestRunner.reset(new QtDBusTest::DBusTestRunner());

    setenv("THUMBNAILER_MAX_IDLE", "bad_value", true);

    dbusService.reset(new QtDBusTest::QProcessDBusService(
            BUS_NAME, QDBusConnection::SessionBus,
            TESTBINDIR "/../src/service/thumbnailer-service",
            QStringList()));

    dbusTestRunner->registerService(dbusService);
    dbusTestRunner->startServices();

    auto process = const_cast<QProcess *>(&dbusService->underlyingProcess());
    if (process->state() != QProcess::NotRunning)
    {
        EXPECT_EQ(process->waitForFinished(), true);
    }
    EXPECT_EQ(process->exitCode(), 1);

    unsetenv("THUMBNAILER_MAX_IDLE");
}

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    setenv("TN_UTILDIR", TESTBINDIR "/../src/vs-thumb", true);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
