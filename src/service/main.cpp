#include <cstdio>

#include <QCoreApplication>

#include "dbusinterface.h"
#include "dbusinterfaceadaptor.h"

using namespace unity::thumbnailer::service;

static const char BUS_NAME[] = "com.canonical.Thumbnailer";
static const char BUS_PATH[] = "/com/canonical/Thumbnailer";

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);

    auto bus = QDBusConnection::sessionBus();

    unity::thumbnailer::service::DBusInterface server;
    new ThumbnailerAdaptor(&server);
    bus.registerObject(BUS_PATH, &server);

    if (!bus.registerService(BUS_NAME)) {
        fprintf(stderr, "Could no acquire D-Bus name %s.\n", BUS_NAME);
        return 0;
    }
    return app.exec();
}
