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

#pragma once

#include <internal/artdownloader.h>
#include <internal/httpdownloader.h>

#include <memory>
#include <string>

namespace unity
{

namespace thumbnailer
{

namespace internal
{

class UbuntuServerDownloader final : public ArtDownloader
{
public:
    UbuntuServerDownloader();
    UbuntuServerDownloader(HttpDownloader* o);  // Takes ownership.
    ~UbuntuServerDownloader() = default;

    std::string download(std::string const& artist, std::string const& album) override;
    std::string download_artist(std::string const& artist, std::string const& album) override;

private:
    std::string download(std::string const& url);
    void set_api_key();

    std::unique_ptr<HttpDownloader> dl;
    std::string api_key;
};

}  // namespace internal

}  // namespace thumbnailer

}  // namespace unity
