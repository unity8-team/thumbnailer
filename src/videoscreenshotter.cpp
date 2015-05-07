/*
 * Copyright (C) 2013 Canonical Ltd.
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
 */

#include <internal/videoscreenshotter.h>
#include <internal/config.h>
#include <internal/file_io.h>

#include <QProcess>
#include <QTemporaryFile>
#include <QTimer>

#include <stdexcept>

using namespace std;
//using namespace unity::thumbnailer::internal;

struct VideoScreenshotterPrivate
{
    string filename;
    bool success = false;
    string error;
    string data;

    QProcess process;
    QTimer timer;
    QTemporaryFile tmpfile;

    VideoScreenshotterPrivate(string const& filename) : filename(filename) {}
};

VideoScreenshotter::VideoScreenshotter(string const& filename)
    : p(new VideoScreenshotterPrivate(filename))
{
    p->process.setStandardInputFile(QProcess::nullDevice());
    p->process.setProcessChannelMode(QProcess::ForwardedChannels);
    p->timer.setSingleShot(true);
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
    p->process.start(exe_path,
                     {QString::fromStdString(p->filename), p->tmpfile.fileName()});
    // Set a watchdog timer in case vs-thumb doesn't finish in time.
    p->timer.start(10000);
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


void VideoScreenshotter::processFinished(int /*exitCode*/, QProcess::ExitStatus /*exitStatus*/)
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
