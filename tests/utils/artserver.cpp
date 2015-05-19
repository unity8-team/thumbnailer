#include "artserver.h"
#include <testsetup.h>

#include <QDebug>
#include <stdexcept>
#include <stdlib.h>

ArtServer::ArtServer()
{
    server_.setStandardInputFile(QProcess::nullDevice());
    server_.setProcessChannelMode(QProcess::ForwardedErrorChannel);
    server_.start(FAKE_ART_SERVER, QStringList());
    if (!server_.waitForStarted())
    {
        throw std::runtime_error("ArtServer::ArtServer(): wait for server start failed");
    }
    if (!server_.waitForReadyRead())
    {
        throw std::runtime_error("ArtServer::ArtServer(): wait for read from server failed");
    }
    auto port = QString::fromUtf8(server_.readAllStandardOutput()).trimmed();
    apiroot_ = "http://127.0.0.1:" + port.toStdString();
    setenv("THUMBNAILER_UBUNTU_APIROOT", apiroot_.c_str(), true);
}

ArtServer::~ArtServer()
{
    server_.terminate();
    if (!server_.waitForFinished())
    {
        qCritical() << "Failed to terminate fake art server";
    }
    unsetenv("THUMBNAILER_UBUNTU_APIROOT");
}
