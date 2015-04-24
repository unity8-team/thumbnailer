#include <memory>
#include <stdexcept>
#include <string>
#include <unistd.h>

#include <libqtdbustest/DBusTestRunner.h>
#include <libqtdbustest/QProcessDBusService.h>

#include <QGuiApplication>
#include <QtQml>
#include <QtQuickTest/quicktest.h>

#include <testsetup.h>


static const char BUS_NAME[] = "com.canonical.Thumbnailer";

class TestFixture
{
public:
    TestFixture() {
        cachedir = TESTBINDIR "/qml-test.XXXXXX";
        if (mkdtemp(const_cast<char*>(cachedir.data())) == nullptr) {
            cachedir = "";
            throw std::runtime_error("Could not create temporary directory");
        }
        setenv("XDG_CACHE_HOME", cachedir.c_str(), true);

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
        if (!cachedir.empty()) {
            std::string cmd = "rm -rf \"" + cachedir + "\"";
            if (system(cmd.c_str()) != 0) {
                throw std::runtime_error("Could not remove temporary directory");
            }
        }
    }

private:
    std::string cachedir;
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
    TestFixture fixture;

    qmlRegisterSingletonType("testconfig", 1, 0, "Config", make_test_config);
    qmlProtectModule("testconfig", 1);
    return quick_test_main(argc, argv, "Thumbnailer", TESTSRCDIR "/qml");
}
