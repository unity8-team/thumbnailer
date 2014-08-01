/*
 * Copyright (C) 2014 Canonical Ltd.
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
 * Authored by: Pawel Stolowski <pawel.stolowski@canonical.com>
 */

#ifndef ART_DOWNLOADER_H
#define ART_DOWNLOADER_H

#include <string>

class ArtDownloader {
public:
    ArtDownloader() = default;
    virtual ~ArtDownloader() = default;
    virtual bool download(const std::string &artist, const std::string &album, const std::string &fname) = 0;
    virtual bool download_artist(const std::string &artist, const std::string &album, const std::string &fname) = 0;
    ArtDownloader(const ArtDownloader&) = delete;
    ArtDownloader& operator=(const ArtDownloader&) = delete;
};

#endif
