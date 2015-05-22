#include "dbusserver.h"
#include <testsetup.h>

namespace
{
const char BUS_NAME[] = "com.canonical.Thumbnailer";

const char THUMBNAILER_BUS_PATH[] = "/com/canonical/Thumbnailer";
const char ADMIN_BUS_PATH[] = "/com/canonical/ThumbnailerAdmin";
}

DBusServer::DBusServer()
{
    service_.reset(new QtDBusTest::QProcessDBusService(
        BUS_NAME, QDBusConnection::SessionBus,
        TESTBINDIR "/../src/service/thumbnailer-service", {}));
    runner_.registerService(service_);
    runner_.startServices();

    thumbnailer.reset(
        new ThumbnailerInterface(BUS_NAME, THUMBNAILER_BUS_PATH,
                                 runner_.sessionConnection()));
    admin.reset(
        new AdminInterface(BUS_NAME, ADMIN_BUS_PATH,
                           runner_.sessionConnection()));
}

DBusServer::~DBusServer()
{
    // If the service is running, give it a chance to shut down.
    // Without this, it won't update the coverage stats.
    if (process().state() == QProcess::Running)
    {
        admin->Shutdown().waitForFinished();
        process().waitForFinished();
    }
    thumbnailer.reset();
    admin.reset();
}

QProcess& DBusServer::process()
{
    // Ugly, but we need access to some of the non-const methods
    return const_cast<QProcess&>(service_->underlyingProcess());
}
