/*
 * Copyright (C) 2013 Canonical Ltd.
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

#ifndef THUMBNAILCACHE_H_
#define THUMBNAILCACHE_H_

#include<thumbnailer.h>
#include<string>

class ThumbnailCachePrivate;

class ThumbnailCache final {
public:
    ThumbnailCache();
    ~ThumbnailCache();

    ThumbnailCache(const ThumbnailCache &t) = delete;
    ThumbnailCache & operator=(const ThumbnailCache &t) = delete;
    ThumbnailCache(ThumbnailCache &&t) = delete;
    ThumbnailCache & operator=(ThumbnailCache &&t) = delete;

    std::string get_if_exists(const std::string &abs_path, ThumbnailSize desired_size) const;
    std::string get_cache_file_name(const std::string &as_path, ThumbnailSize desired_size) const;
    void clear();
    void prune();

private:
    ThumbnailCachePrivate *const p;
};


#endif
