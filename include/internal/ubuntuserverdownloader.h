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
 *              Xavi Garcia <xavi.garcia.mena@canonical.com>
 */

#pragma once

#include <internal/artdownloader.h>

#include <QNetworkAccessManager>

#include <map>
#include <memory>

namespace unity
{

namespace thumbnailer
{

namespace internal
{

class ArtReply;
class UbuntuServerArtReply;

class UbuntuServerDownloader final : public ArtDownloader
{
    Q_OBJECT
public:
    Q_DISABLE_COPY(UbuntuServerDownloader)

    explicit UbuntuServerDownloader(QObject* parent = nullptr);
    ~UbuntuServerDownloader() = default;

    std::shared_ptr<ArtReply> download_album(QString const& artist, QString const& album) override;
    std::shared_ptr<ArtReply> download_artist(QString const& artist, QString const& album) override;

protected Q_SLOTS:
    void download_finished(QNetworkReply* reply);

private:
    void set_api_key();
    std::shared_ptr<ArtReply> download_url(QUrl const& url);

    QString api_key_;
    QNetworkAccessManager network_manager_;
    std::map<QNetworkReply *, std::shared_ptr<UbuntuServerArtReply>> replies_map_;
};

}  // namespace internal

}  // namespace thumbnailer

}  // namespace unity
