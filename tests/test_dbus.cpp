
#include <memory>
#include <stdexcept>
#include <string>
#include <cstdlib>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <gtest/gtest.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <libqtdbustest/DBusTestRunner.h>
#include <libqtdbustest/QProcessDBusService.h>
#include <unity/util/ResourcePtr.h>

#include <testsetup.h>

static const char BUS_NAME[] = "com.canonical.Thumbnailer";
static const char BUS_PATH[] = "/com/canonical/Thumbnailer";
static const char THUMBNAILER_IFACE[] = "com.canonical.Thumbnailer";


typedef unity::util::ResourcePtr<int, decltype(&::close)> FdPtr;
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
        tempdir = TESTBINDIR "/dbus-test.XXXXXX";
        if (mkdtemp(const_cast<char*>(tempdir.data())) == nullptr) {
            tempdir = "";
            throw std::runtime_error("could not create temporary directory");
        }
        setenv("XDG_CACHE_HOME", (tempdir + "/cache").c_str(), true);

        dbusTestRunner.reset(new QtDBusTest::DBusTestRunner());
        dbusTestRunner->registerService(
            QtDBusTest::DBusServicePtr(
                new QtDBusTest::QProcessDBusService(
                    BUS_NAME, QDBusConnection::SessionBus,
                    TESTBINDIR "/../src/service/thumbnailer-service",
                    QStringList())));
        dbusTestRunner->startServices();

        iface.reset(
            new QDBusInterface(BUS_NAME, BUS_PATH, THUMBNAILER_IFACE,
                               dbusTestRunner->sessionConnection()));
    }

    virtual void TearDown() override {
        iface.reset();
        dbusTestRunner.reset();

        if (!tempdir.empty()) {
            std::string cmd = "rm -rf \"" + tempdir + "\"";
            ASSERT_EQ(system(cmd.c_str()), 0);
        }
    }

    std::string tempdir;
    std::unique_ptr<QtDBusTest::DBusTestRunner> dbusTestRunner;
    std::unique_ptr<QDBusInterface> iface;
};

/* Disabled until we have the fake art service hooked up */
/*
TEST_F(DBusTest, get_album_art) {
    QDBusReply<QDBusUnixFileDescriptor> reply = iface->call(
        "GetAlbumArt", "Radiohead", "OK Computer", "large");
    assert_no_error(reply);
}

TEST_F(DBusTest, get_album_art) {
    QDBusReply<QDBusUnixFileDescriptor> reply = iface->call(
        "GetArtistArt", "Radiohead", "OK Computer", "large");
    assert_no_error(reply);
}
*/

TEST_F(DBusTest, thumbnail_image) {
    const char *filename = TESTDATADIR "/testimage.jpg";
    FdPtr fd(open(filename, O_RDONLY), close);
    ASSERT_GE(fd.get(), 0);

    QDBusReply<QDBusUnixFileDescriptor> reply = iface->call(
        "GetThumbnail", filename, QVariant::fromValue(QDBusUnixFileDescriptor(fd.get())), "large");
    assert_no_error(reply);

    auto pixbuf = read_image(reply.value());
    EXPECT_EQ(256, gdk_pixbuf_get_width(pixbuf.get()));
    EXPECT_EQ(160, gdk_pixbuf_get_height(pixbuf.get()));
}

TEST_F(DBusTest, thumbnail_no_such_file) {
    const char *no_such_file = TESTDATADIR "/no-such-file.jpg";
    const char *filename2 = TESTDATADIR "/testrotate.jpg";

    FdPtr fd(open(filename2, O_RDONLY), close);
    ASSERT_GE(fd.get(), 0);

    QDBusReply<QDBusUnixFileDescriptor> reply = iface->call(
        "GetThumbnail", no_such_file, QVariant::fromValue(QDBusUnixFileDescriptor(fd.get())), "large");
    EXPECT_FALSE(reply.isValid());
    auto message = reply.error().message().toStdString();
    EXPECT_EQ(message, "Could not stat file");
}

TEST_F(DBusTest, thumbnail_wrong_fd_fails) {
    const char *filename1 = TESTDATADIR "/testimage.jpg";
    const char *filename2 = TESTDATADIR "/testrotate.jpg";

    FdPtr fd(open(filename2, O_RDONLY), close);
    ASSERT_GE(fd.get(), 0);

    QDBusReply<QDBusUnixFileDescriptor> reply = iface->call(
        "GetThumbnail", filename1, QVariant::fromValue(QDBusUnixFileDescriptor(fd.get())), "large");
    EXPECT_FALSE(reply.isValid());
    auto message = reply.error().message().toStdString();
    EXPECT_EQ(message, "filename refers to a different file than the file descriptor");
}

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    setenv("TN_UTILDIR", TESTBINDIR "/../src/vs-thumb", true);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
