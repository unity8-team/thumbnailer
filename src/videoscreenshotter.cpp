/*
 * Copyright (C) 2013-2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Jussi Pakkanen <jussi.pakkanen@canonical.com>
 *              James Henstridge <james.henstridge@canonical.com>
 */

#include <internal/videoscreenshotter.h>
#include <internal/config.h>
#include <internal/file_io.h>
#include <internal/raii.h>
#include <internal/safe_strerror.h>

#include <QProcess>
#include <QTemporaryFile>
#include <QTimer>

#include <stdexcept>
#include <unistd.h>

using namespace std;
using namespace unity::thumbnailer::internal;

struct VideoScreenshotterPrivate
{
    FdPtr fd;
    int timeout_ms;
    bool success = false;
    string error;
    string data;

    QProcess process;
    QTimer timer;
    QTemporaryFile tmpfile;

    VideoScreenshotterPrivate(int fd, chrono::milliseconds timeout)
        : fd(fd, do_close)
        , timeout_ms(timeout.count())
    {
    }
};

VideoScreenshotter::VideoScreenshotter(int fd, chrono::milliseconds timeout)
    : p(new VideoScreenshotterPrivate(dup(fd), timeout))
{
    if (p->fd.get() < 0)
    {
        throw runtime_error("VideoScreenshotter(): could not duplicate fd: " + safe_strerror(errno));
    }

    p->process.setStandardInputFile(QProcess::nullDevice());
    p->process.setProcessChannelMode(QProcess::ForwardedChannels);
    connect(&p->process, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), this, &VideoScreenshotter::processFinished);
    connect(&p->timer, &QTimer::timeout, this, &VideoScreenshotter::timeout);
}

VideoScreenshotter::~VideoScreenshotter() = default;

void VideoScreenshotter::extract()
{
    // Gstreamer video pipelines are unstable so we need to run an
    // external helper library.
    char *utildir = getenv("TN_UTILDIR");
    QString exe_path = utildir ? utildir : SHARE_PRIV_ABS;
    exe_path += "/vs-thumb";

    if (!p->tmpfile.open()) {
        throw runtime_error("VideoScreenshotter::extract: unable to open temporary file");
    }
    /* Our duplicated file descriptor does not have the FD_CLOEXEC flag set */
    p->process.start(exe_path,
                     {QString("fd://%1").arg(p->fd.get()), p->tmpfile.fileName()});
    // Set a watchdog timer in case vs-thumb doesn't finish in time.
    p->timer.start(15000);
}

bool VideoScreenshotter::success()
{
    return p->process.exitStatus() == QProcess::NormalExit &&
        p->process.exitCode() == 0;
}

string VideoScreenshotter::error()
{
    switch (p->process.exitStatus())
    {
    case QProcess::NormalExit:
        switch (p->process.exitCode())
        {
        case 0:
            return "";
        case 1:
            return "Could not extract screenshot";
        case 2:
            return "Video extractor pipeline failed";
        default:
            return string("Unknown error when trying to extract video screenshot, return value was ") + to_string(p->process.exitCode());
        }
    case QProcess::CrashExit:
        return "vs-thumb subprocess crashed";
    default:
        abort(); // Impossible
    }
}

string VideoScreenshotter::data()
{
    return read_file(p->tmpfile.fileName().toStdString());
}


void VideoScreenshotter::processFinished()
{
    p->timer.stop();
    Q_EMIT finished();
}

void VideoScreenshotter::timeout()
{
    if (p->process.state() != QProcess::NotRunning)
    {
        p->process.kill();
    }
}
