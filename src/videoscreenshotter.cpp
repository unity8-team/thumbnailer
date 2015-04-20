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

#include <internal/config.h>
#include <internal/videoscreenshotter.h>
#include <internal/safe_strerror.h>

#include <cerrno>
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
#include <stdexcept>
#include <cstring>
#include <sys/time.h>
#include <signal.h>
#include <time.h>

using namespace std;
using namespace unity::thumbnailer::internal;

static double timestamp() {
    struct timeval now;
    gettimeofday(&now, NULL);
    return now.tv_sec + now.tv_usec/1000000.0;

}

static bool wait_for_helper(pid_t child) {
    const double max_wait_time = 10;
    struct timespec sleep_time;
    double start = timestamp();
    sleep_time.tv_sec = 0;
    sleep_time.tv_nsec = 100000000;
    int status;
    while(waitpid(child, &status, WNOHANG) == 0) {
        struct timespec dummy;
        double now = timestamp();
        if(now - start >= max_wait_time) {
            if(kill(child, SIGKILL) < 0) {
                string msg("Could not kill child process: ");
                msg += safe_strerror(errno);
                throw runtime_error(msg);
            }
            waitpid(child, &status, 0);
            throw runtime_error("Helper process took too long.");
        }
        nanosleep(&sleep_time, &dummy);
    }
    if(status < 0) {
        throw runtime_error("Waiting for child process failed.");
    }
    if(status == 0) {
        return true;
    }
    if(status == 1) {
        return false;
    }
    if(status == 2) {
        throw runtime_error("Video extractor pipeline failed");
    }
    string errmsg("Unknown error when trying to extract video screenshot, return value was ");
    errmsg += std::to_string(status);
    errmsg += ".";
    throw runtime_error(errmsg);
}

VideoScreenshotter::VideoScreenshotter() {
}

VideoScreenshotter::~VideoScreenshotter() {
}

bool VideoScreenshotter::extract(const std::string &ifname, const std::string &ofname) {
    // Gstreamer video pipelines are unstable so we need to run an
    // external helper library.
    string exe_path;
    char *utildir = getenv("TN_UTILDIR");
    exe_path = utildir ? utildir : SHARE_PRIV_ABS;
    string cmd(exe_path + "/vs-thumb");
    pid_t child = fork();
    if(child == -1) {
        throw runtime_error("Could not spawn worker process.");
    }
    if(child == 0) {
        execl(cmd.c_str(), cmd.c_str(), ifname.c_str(), ofname.c_str(), (char*) NULL);
        fprintf(stderr, "Could not execute worker process: %s", safe_strerror(errno).c_str());
        _exit(100);
    } else {
        try {
            return wait_for_helper(child);
        } catch(...) {
            unlink(ofname.c_str());
            throw;
        }
    }
    throw runtime_error("Code that should not have been reached was reached.");
}

