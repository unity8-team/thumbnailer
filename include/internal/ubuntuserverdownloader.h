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

#ifndef UBUNTUSERVER_DOWNLOADER_H
#define UBUNTUSERVER_DOWNLOADER_H

#include <string>
#include <memory>
#include <internal/httpdownloader.h>
#include <internal/artdownloader.h>

class UbuntuServerDownloader final : public ArtDownloader {
public:
    UbuntuServerDownloader();
    UbuntuServerDownloader(HttpDownloader *o); // Takes ownership.
    ~UbuntuServerDownloader() = default;

    bool download(const std::string &artist, const std::string &album, const std::string &fname) override;
    bool download_artist(const std::string &artist, const std::string &fname) override;

private:
    bool download(const std::string &url, const std::string &fname);
    std::unique_ptr<HttpDownloader> dl;
};

#endif
