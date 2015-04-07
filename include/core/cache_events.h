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
 * Authored by: Michi Henning <michi.henning@canonical.com>
 */

#pragma once

namespace core
{

/**
\brief Event types that can be monitored.
*/

// Note: Any change here must have a corresponding change to
//       CacheEventIndex in core/internal/cache_event_indexes.h!

enum class CacheEvent : unsigned
{
    Get        = 1 << 0,  ///< An entry was returned by a call to get(), get_or_put(), or take().
    Put        = 1 << 1,  ///< An entry was added by a call to put() or get_or_put().
    Invalidate = 1 << 2,  ///< An entry was removed by a call to invalidate() or take().
    Touch      = 1 << 3,  ///< An entry was refreshed by a call to touch().
    Miss       = 1 << 4,  ///< A call to get(), get_or_put(), or take() failed to return an entry.
    Evict_TTL  = 1 << 5,  ///< An expired entry was evicted due to a call to put(),
                          ///< get_or_put(), trim_to(), or resize().
    Evict_LRU  = 1 << 6,  ///< The oldest entry was evicted due to a call to put(),
                          ///< get_or_put(), trim_to(), or resize().
    END_       = 1 << 7   ///< End marker
};

/**
\brief Convenience definition for all event types.
*/

static constexpr unsigned AllCacheEvents = static_cast<unsigned>(CacheEvent::END_) - 1;

}  // namespace core
