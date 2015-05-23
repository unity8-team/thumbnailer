#include <memory>
#include <stdexcept>
#include <string>
#include <unistd.h>

#include <QGuiApplication>
#include <QtQml>
#include <QtQuickTest/quicktest.h>
#include <QTemporaryDir>

#include <testsetup.h>
#include "utils/artserver.h"
#include "utils/dbusserver.h"

class TestFixture
{
public:
    TestFixture()
    {
        cachedir_.reset(new QTemporaryDir(TESTBINDIR "/qml-test.XXXXXX"));
        setenv("XDG_CACHE_HOME", cachedir_->path().toUtf8().constData(), true);

        dbus_server_.reset(new DBusServer());
    }

    ~TestFixture()
    {
        dbus_server_.reset();
        cachedir_.reset();
    }

private:
    std::unique_ptr<QTemporaryDir> cachedir_;
    std::unique_ptr<DBusServer> dbus_server_;
    ArtServer fake_art_server_;
};

// Expose static test configuration to QML
QJSValue make_test_config(QQmlEngine*, QJSEngine* scriptEngine)
{
    QJSValue config = scriptEngine->newObject();
    config.setProperty("sourceDir", TESTSRCDIR);
    config.setProperty("buildDir", TESTBINDIR);
    config.setProperty("mediaDir", TESTDATADIR);
    return config;
}

int main(int argc, char** argv)
{
    QGuiApplication app(argc, argv);

    setenv("TN_UTILDIR", TESTBINDIR "/../src/vs-thumb", true);
    qmlRegisterSingletonType("testconfig", 1, 0, "Config", make_test_config);
    qmlProtectModule("testconfig", 1);

    TestFixture fixture;
    return quick_test_main(argc, argv, "Thumbnailer", TESTSRCDIR "/qml");
}
