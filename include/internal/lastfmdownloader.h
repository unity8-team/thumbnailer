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
 *              Xavi Garcia <xavi.garcia.mena@canonical.com>
 */

#pragma once

#include <internal/artdownloader.h>

#include <QNetworkAccessManager>


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
    Q_DISABLE_COPY(LastFMDownloader)

    explicit LastFMDownloader(QObject* parent = nullptr);
    ~LastFMDownloader() = default;

    ArtReply * download_album(QString const& artist, QString const& album) override;
    ArtReply * download_artist(QString const& artist, QString const& album) override;

protected Q_SLOTS:
    void download_finished(QNetworkReply* reply);

private:
    typedef QMap<QNetworkReply *, ArtReply *>::iterator IterReply;

    void download_xml_finished(QNetworkReply* reply, IterReply const& iter);
    void download_image_finished(QNetworkReply* reply, IterReply const& iter);

    QNetworkAccessManager network_manager_;
    QMap<QNetworkReply *, ArtReply *> replies_xml_map_;
    QMap<QNetworkReply *, ArtReply *> replies_image_map_;
};

}  // namespace internal

}  // namespace thumbnailer

}  // namespace unity
