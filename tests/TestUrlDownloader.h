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

#include "../include/internal/qartdownloader.h"

namespace unity
{

namespace thumbnailer
{

namespace internal
{

namespace test
{

class TestUrlDownloader : public QArtDownloader
{
    Q_OBJECT
public:
    TestUrlDownloader(QObject* parent = nullptr);
    ~TestUrlDownloader() = default;

    QString download(QString const& artist, QString const& album) override;
    QString download_artist(QString const& artist, QString const& album) override;

    QString download_url(QUrl const& url);
};

}  // namespace test

}  // namespace internal

}  // namespace thumbnailer

}  // namespace unity
