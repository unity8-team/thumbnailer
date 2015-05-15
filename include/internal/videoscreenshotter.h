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

#pragma once

#include <QProcess>
#include <QTemporaryFile>
#include <QTimer>
#include <unity/util/ResourcePtr.h>

#include <chrono>
#include <memory>
#include <string>

struct VideoScreenshotterPrivate;

class VideoScreenshotter final : public QObject
{
    Q_OBJECT
public:
    VideoScreenshotter(int fd, std::chrono::milliseconds timeout);
    ~VideoScreenshotter();

    VideoScreenshotter(const VideoScreenshotter& t) = delete;
    VideoScreenshotter& operator=(const VideoScreenshotter& t) = delete;

    void extract();
    bool success();
    std::string error();
    std::string data();

Q_SIGNALS:
    void finished();

private Q_SLOTS:
    void processFinished();
    void timeout();

private:
    unity::util::ResourcePtr<int, void (*)(int)> fd_;
    int timeout_ms_;
    bool success_ = false;
    std::string error_;
    std::string data_;

    QProcess process_;
    QTimer timer_;
    QTemporaryFile tmpfile_;
};
