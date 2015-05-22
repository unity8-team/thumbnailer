#pragma once

#include <utils/thumbnailerinterface.h>
#include <utils/admininterface.h>

#include <QProcess>
#include <QSharedPointer>

#include <memory>

namespace QtDBusTest
{
class DBusTestRunner;
class QProcessDBusService;
}

class DBusServer final {
public:
    DBusServer();
    ~DBusServer();

    std::unique_ptr<ThumbnailerInterface> thumbnailer_;
    std::unique_ptr<AdminInterface> admin_;

    QProcess& service_process();

private:
    std::unique_ptr<QtDBusTest::DBusTestRunner> runner_;
    QSharedPointer<QtDBusTest::QProcessDBusService> service_;
};
