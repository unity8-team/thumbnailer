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
 * Authored by: Jussi Pakkanen <jussi.pakkanen@canonical.com>
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

class LastFMDownloader final : public ArtDownloader
{
public:
    LastFMDownloader();
    LastFMDownloader(HttpDownloader* o);  // Takes ownership.
    ~LastFMDownloader() = default;
    LastFMDownloader(LastFMDownloader const& o) = delete;
    LastFMDownloader& operator=(LastFMDownloader const& o) = delete;

    std::string download(std::string const& artist, std::string const& album) override;
    std::string download_artist(std::string const& artist, std::string const& album) override;

private:
    std::string parse_xml(std::string const& xml);
    std::unique_ptr<HttpDownloader> dl;
};

}  // namespace internal

}  // namespace thumbnailer

}  // namespace unity

