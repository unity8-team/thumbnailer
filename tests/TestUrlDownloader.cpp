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

#include <TestUrlDownloader.h>

using namespace unity::thumbnailer::internal::test;
using namespace unity::thumbnailer::internal;

TestUrlDownloader::TestUrlDownloader(QObject* parent)
    : QArtDownloader(parent)
{
}

QString TestUrlDownloader::download(QString const&, QString const&)
{
    // does nothing, this class is only intended for testing purposes
    return "";
}

QString TestUrlDownloader::download_artist(QString const&, QString const&)
{
    // does nothing, this class is only intended for testing purposes
    return "";
}

QString TestUrlDownloader::download_url(QUrl const& url)
{
    // starts the download
    return start_download(url)->url().toString();
}
