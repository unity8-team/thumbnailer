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

#pragma once

#include <memory>
#include <string>
#include <QObject>
#include <QProcess>

struct VideoScreenshotterPrivate;

class VideoScreenshotter final : public QObject {
    Q_OBJECT
public:
    VideoScreenshotter(const std::string &filename);
    ~VideoScreenshotter();

    VideoScreenshotter(const VideoScreenshotter &t) = delete;
    VideoScreenshotter & operator=(const VideoScreenshotter &t) = delete;

    void extract();
    bool success();
    std::string error();
    std::string data();

Q_SIGNALS:
    void finished();

private Q_SLOTS:
    void processFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void timeout();

private:
    std::unique_ptr<VideoScreenshotterPrivate> p;
};
