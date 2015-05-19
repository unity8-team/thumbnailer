#pragma once

#include <string>
#include <QProcess>

class ArtServer final {
public:
    ArtServer();
    ~ArtServer();

    std::string apiroot();

private:
    QProcess server_;
    std::string apiroot_;
};
