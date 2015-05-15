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

#include <unistd.h>

using namespace std;
using namespace unity::thumbnailer::internal;

VideoScreenshotter::VideoScreenshotter(int fd, chrono::milliseconds timeout)
    : fd_(dup(fd), do_close)
    , timeout_ms_(timeout.count())
{
    if (fd_.get() < 0)
    {
        throw runtime_error("VideoScreenshotter(): could not duplicate fd: " + safe_strerror(errno));
    }

    process_.setStandardInputFile(QProcess::nullDevice());
    process_.setProcessChannelMode(QProcess::ForwardedChannels);
    connect(&process_, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), this,
            &VideoScreenshotter::processFinished);
    connect(&timer_, &QTimer::timeout, this, &VideoScreenshotter::timeout);
}

VideoScreenshotter::~VideoScreenshotter() = default;

void VideoScreenshotter::extract()
{
    // Gstreamer video pipelines are unstable so we need to run an
    // external helper library.
    char* utildir = getenv("TN_UTILDIR");
    QString exe_path = utildir ? utildir : SHARE_PRIV_ABS;
    exe_path += "/vs-thumb";

    if (!tmpfile_.open())
    {
        throw runtime_error("VideoScreenshotter::extract: unable to open temporary file");
    }
    /* Our duplicated file descriptor does not have the FD_CLOEXEC flag set */
    process_.start(exe_path, {QString("fd://%1").arg(fd_.get()), tmpfile_.fileName()});
    // Set a watchdog timer in case vs-thumb doesn't finish in time.
    timer_.start(15000);
}

bool VideoScreenshotter::success()
{
    return process_.exitStatus() == QProcess::NormalExit && process_.exitCode() == 0;
}

string VideoScreenshotter::error()
{
    switch (process_.exitStatus())
    {
        case QProcess::NormalExit:
            switch (process_.exitCode())
            {
                case 0:
                    return "";
                case 1:
                    return "Could not extract screenshot";
                case 2:
                    return "Video extractor pipeline failed";
                default:
                    return string("Unknown error when trying to extract video screenshot, return value was ") +
                           to_string(process_.exitCode());
            }
        case QProcess::CrashExit:
            return "vs-thumb subprocess crashed";
        default:
            abort();  // Impossible
    }
}

string VideoScreenshotter::data()
{
    return read_file(tmpfile_.fileName().toStdString());
}

void VideoScreenshotter::processFinished()
{
    timer_.stop();
    Q_EMIT finished();
}

void VideoScreenshotter::timeout()
{
    if (process_.state() != QProcess::NotRunning)
    {
        process_.kill();
    }
}
