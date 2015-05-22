#pragma once

#include <utils/thumbnailerinterface.h>
#include <utils/admininterface.h>

#include <memory>

#include <libqtdbustest/DBusTestRunner.h>
#include <libqtdbustest/QProcessDBusService.h>
#include <QProcess>

class DBusServer final {
public:
    DBusServer();
    ~DBusServer();

    std::unique_ptr<ThumbnailerInterface> thumbnailer;
    std::unique_ptr<AdminInterface> admin;

    QProcess& process();

private:
    QtDBusTest::DBusTestRunner runner_;
    QSharedPointer<QtDBusTest::QProcessDBusService> service_;
};
