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

#include <memory>

namespace unity
{

namespace thumbnailer
{

namespace internal
{

class UbuntuServerDownloader final : public ArtDownloader
{
    Q_OBJECT
public:
    UbuntuServerDownloader(QObject* parent = nullptr);
    ~UbuntuServerDownloader() = default;

    QString download_album(QString const& artist, QString const& album) override;
    QString download_artist(QString const& artist, QString const& album) override;

    UbuntuServerDownloader(UbuntuServerDownloader const&) = delete;
    UbuntuServerDownloader& operator=(UbuntuServerDownloader const&) = delete;

private:
    void set_api_key();

    QString api_key;
};

}  // namespace internal

}  // namespace thumbnailer

}  // namespace unity
