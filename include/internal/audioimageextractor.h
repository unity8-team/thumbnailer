/*
 * Copyright (C) 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Jussi Pakkanen <jussi.pakkanen@canonical.com>
 */

#ifndef AUDIOIMAGEEXTRACTOR_H_
#define AUDIOIMAGEEXTRACTOR_H_

#include<string>

class AudioImageExtractorPrivate;

class AudioImageExtractor {
public:
    AudioImageExtractor();
    ~AudioImageExtractor();

    AudioImageExtractor(const AudioImageExtractor &t) = delete;
    AudioImageExtractor & operator=(const AudioImageExtractor &t) = delete;

    bool extract(const std::string &ifname, const std::string &ofname);

private:
    AudioImageExtractorPrivate *p;
};


#endif
