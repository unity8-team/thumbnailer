/*
 * Copyright (C) 2015 Canonical, Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of version 3 of the GNU General Public License as published
 * by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *    James Henstridge <james.henstridge@canonical.com>
 */

#pragma once

#include <functional>
#include <queue>

namespace unity
{

namespace thumbnailer
{

namespace service
{

class RateLimiter {
public:
    RateLimiter(int concurrency);
    ~RateLimiter();

    void schedule(std::function<void()> job);
    void done();

private:
    int const concurrency_;
    int running_;
    std::queue<std::function<void()>> queue_;
};

}  // namespace service

}  // namespace thumbnailer

}  // namespace unity
