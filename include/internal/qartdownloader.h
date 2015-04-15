/*
 * Copyright (C) 2015 Canonical Ltd
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
 * Authored by: Xavi Garcia <xavi.garcia.mena@canonical.com>
 */

#pragma once

#include <internal/qurldownloader.h>

namespace unity
{

namespace thumbnailer
{

namespace internal
{

/**
\brief Base class to download remote files.

This class uses internally QNetworkAccessManager to retrieve remote urls.
*/
class QArtDownloader : public QUrlDownloader
{
    Q_OBJECT
public:
    QArtDownloader(QObject* parent = nullptr) : QUrlDownloader(parent){};
    virtual ~QArtDownloader() = default;

    QArtDownloader(QArtDownloader const&) = delete;
    QArtDownloader& operator=(QArtDownloader const&) = delete;

    /**
    \brief Downloads the image for the given artist and album.
     After this call expect file_downloaded,  download_error, download_source_not_found
     or bad_url_error signals to be emitted

     \note the url is also returned in the file_downloaded and download_error signals
     so you can identify the url being downloaded or giving an error in case
     you run from multiple threads or download multiple files.

     \return the url being downloaded or an empty QString if the constructed is not
             valid
    */
    virtual QString download(QString const& artist, QString const& album) = 0;

    /**
    \brief Downloads the image of the artist for the given artist and album.
     After this call expect file_downloaded,  download_error, download_source_not_found
     or bad_url_error signals to be emitted

     \note the url is also returned in the file_downloaded and download_error signals
     so you can identify the url being downloaded or giving an error in case
     you run from multiple threads or download multiple files.

     \return the url being downloaded or an empty QString if the constructed is not
             valid
    */
    virtual QString download_artist(QString const& artist, QString const& album) = 0;
};

}  // namespace internal

}  // namespace thumbnailer

}  // namespace unity
