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

#include<internal/videoscreenshotter.h>

#include<unistd.h>
#include<sys/wait.h>
#include<stdexcept>
#include<cstring>

using namespace std;

class VideoScreenshotterPrivate {
};

VideoScreenshotter::VideoScreenshotter() {
    p = new VideoScreenshotterPrivate();
}

VideoScreenshotter::~VideoScreenshotter() {
    delete p;
}

bool VideoScreenshotter::extract(const std::string &ifname, const std::string &ofname) {
    // Gstreamer video pipelines are unstable so we need to run an
    // external helper library.
    string cmd("/home/jpakkane/workspace/thumbnailer/build/src/vs-thumb");
    pid_t child = fork();
    if(child == -1) {
        throw runtime_error("Could not spawn worker process.");
    }
    if(child  == 0) {
        execl(cmd.c_str(), cmd.c_str(), ifname.c_str(), ofname.c_str(), (char*) NULL);
        printf("Could not execute helper process: %s", strerror(errno));
        _exit(100);
    } else {
        int status;
        waitpid(child, &status, 0);
        if(status == 0) {
            return true;
        }
        if(status == 1) {
            return false;
        }
        if(status == 2) {
            throw runtime_error("Video extractor pipeline failed");
        }
        throw runtime_error("Unknown error when trying to extract video screenshot.");
    }
    throw runtime_error("Code that should not have been reached was reached.");
}

