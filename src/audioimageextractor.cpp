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

#include<cstdio>
#include<internal/audioimageextractor.h>
#include<unistd.h>
#include<stdexcept>
#include<memory>
#include"internal/videoscreenshotter.h"

using namespace std;

class AudioImageExtractorPrivate {

public:

    bool extract(const string &ifname, const string &ofname);

private:
    // Use the external binary to extract the embedded image.
    // We could use the vs object in the Thumbnailer class and remove
    // AudioImageExtractor completely but it is possible that
    // this is refactored to use a dbus service for efficiency
    // so having this class around will make that refactoring easier.
    VideoScreenshotter vs;
};

bool AudioImageExtractorPrivate::extract(const string &ifname, const string &ofname) {
    return vs.extract(ifname, ofname);
}

AudioImageExtractor::AudioImageExtractor() {
    p = new AudioImageExtractorPrivate();
}

AudioImageExtractor::~AudioImageExtractor() {
    delete p;
}


bool AudioImageExtractor::extract(const string &ifname, const string &ofname) {
    return p->extract(ifname, ofname);
}
