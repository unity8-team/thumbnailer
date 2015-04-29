#include <memory>
#include <stdexcept>
#include <string>
#include <unistd.h>

#include <libqtdbustest/DBusTestRunner.h>
#include <libqtdbustest/QProcessDBusService.h>

#include <QGuiApplication>
#include <QtQml>
#include <QtQuickTest/quicktest.h>
#include <QTemporaryDir>

#include <testsetup.h>


static const char BUS_NAME[] = "com.canonical.Thumbnailer";

class TestFixture
{
public:
    TestFixture() {
        cachedir.reset(new QTemporaryDir(TESTBINDIR "/qml-test.XXXXXX"));
        setenv("XDG_CACHE_HOME", cachedir->path().toUtf8().data(), true);

        dbusTestRunner.reset(new QtDBusTest::DBusTestRunner());
        dbusTestRunner->registerService(
            QtDBusTest::DBusServicePtr(
                new QtDBusTest::QProcessDBusService(
                    BUS_NAME, QDBusConnection::SessionBus,
                    TESTBINDIR "/../src/service/thumbnailer-service",
                    QStringList())));
        dbusTestRunner->startServices();
    }

    ~TestFixture() {
        dbusTestRunner.reset();
        cachedir.reset();
    }

private:
    std::unique_ptr<QTemporaryDir> cachedir;
    std::unique_ptr<QtDBusTest::DBusTestRunner> dbusTestRunner;
};

// Expose static test configuration to QML
QJSValue make_test_config(QQmlEngine *, QJSEngine *scriptEngine)
{
    QJSValue config = scriptEngine->newObject();
    config.setProperty("sourceDir", TESTSRCDIR);
    config.setProperty("buildDir", TESTBINDIR);
    config.setProperty("mediaDir", TESTDATADIR);
    return config;
}

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);

    setenv("TN_UTILDIR", TESTBINDIR "/../src/vs-thumb", true);
    qmlRegisterSingletonType("testconfig", 1, 0, "Config", make_test_config);
    qmlProtectModule("testconfig", 1);

    TestFixture fixture;
    return quick_test_main(argc, argv, "Thumbnailer", TESTSRCDIR "/qml");
}
