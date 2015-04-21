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
    Q_OBJECT
public:
    LastFMDownloader(QObject* parent = nullptr);
    ~LastFMDownloader() = default;
    LastFMDownloader(LastFMDownloader const& o) = delete;
    LastFMDownloader& operator=(LastFMDownloader const& o) = delete;

    QString download_album(QString const& artist, QString const& album) override;
    QString download_artist(QString const& artist, QString const& album) override;

Q_SIGNALS:
    void xml_parsing_error(QString const& url, QString const& error_message);

private Q_SLOTS:
    void xml_downloaded(QString const& url, QByteArray const& data);
    void xml_download_error(QString const& url, QNetworkReply::NetworkError error, QString const& error_message);

private:
    QString parse_xml(QString const& source_url, QByteArray const& data);

    // we use this downloader to get only the xml file that defines
    // where to get images.
    QScopedPointer<UrlDownloader> xml_downloader_;

    // we use this map to store the number of retries we allow when
    // downloading an xml.
    QMap<QString, int> retries_map_;
};

}  // namespace internal

}  // namespace thumbnailer

}  // namespace unity
