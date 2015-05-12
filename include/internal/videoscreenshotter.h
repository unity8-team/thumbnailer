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

#include <chrono>
#include <memory>
#include <string>
#include <QObject>

struct VideoScreenshotterPrivate;

class VideoScreenshotter final : public QObject {
    Q_OBJECT
public:
    VideoScreenshotter(const std::string &filename, std::chrono::milliseconds timeout);
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
    void processFinished();
    void timeout();

private:
    std::unique_ptr<VideoScreenshotterPrivate> p;
};
