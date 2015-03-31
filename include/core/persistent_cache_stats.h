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

#include <core/cache_discard_policy.h>

#include <chrono>
#include <memory>

namespace core
{

namespace internal
{

class PersistentStringCacheImpl;
class PersistentStringCacheStats;

}  // namespace internal

/**
\brief Class that provides (read-only) access to cache statistics and settings.
*/

class PersistentCacheStats
{
public:
    /** @name Construction, Copy and Assignment
    Copy and assignment have the usual value semantics.
    The default constructor creates an instance with
    an empty cache path, `LRU_only` policy, and the
    remaining values set to zero.
    */
    //{@
    PersistentCacheStats();
    PersistentCacheStats(PersistentCacheStats const&);
    PersistentCacheStats(PersistentCacheStats&&) noexcept;
    PersistentCacheStats& operator=(PersistentCacheStats const&);
    PersistentCacheStats& operator=(PersistentCacheStats&&);
    //@}

    // Accessors instead of data members for ABI stability.
    /** @name Accessors
    */

    //{@
    /**
    \brief Returns the path to the cache directory.
    */
    std::string cache_path() const;

    /**
    \brief Returns the discard policy (`LRU_only` or `LRU_TTL`).
    */
    CacheDiscardPolicy policy() const noexcept;

    /**
    \brief Returns the number of entries (including expired ones).
    */
    int64_t size() const noexcept;

    /**
    \brief Returns the size of all entries (including expired ones).
    */
    int64_t size_in_bytes() const noexcept;

    /**
    \brief Returns the maximum size of the cache.
    */
    int64_t max_size_in_bytes() const noexcept;

    /**
    \brief Returns the currently set headroom.
    */
    int64_t headroom() const noexcept;

    /**
    \brief Returns the number of hits since the statistics were last reset.
    */
    int64_t hits() const noexcept;

    /**
    \brief Returns the number of misses since the statistics were last reset.
    */
    int64_t misses() const noexcept;

    /**
    \brief Returns the number of consecutive hits since the last miss.
    */
    int64_t hits_since_last_miss() const noexcept;

    /**
    \brief Returns the number of consecutive misses since the last hit.
    */
    int64_t misses_since_last_hit() const noexcept;

    /**
    \brief Returns the largest number of consecutive hits.
    */
    int64_t longest_hit_run() const noexcept;

    /**
    \brief Returns the largest number of consecutive misses.
    */
    int64_t longest_miss_run() const noexcept;

    /**
    \brief Returns the timestamp of the most recent hit.
    */
    std::chrono::steady_clock::time_point most_recent_hit_time() const noexcept;

    /**
    \brief Returns the timestamp of the most recent miss.
    */
    std::chrono::steady_clock::time_point most_recent_miss_time() const noexcept;

    /**
    \brief Returns the time of the longest hit run.
    */
    std::chrono::steady_clock::time_point longest_hit_run_time() const noexcept;

    /**
    \brief Returns the time of the longest miss run.
    */
    std::chrono::steady_clock::time_point longest_miss_run_time() const noexcept;
    //@}

private:
    PersistentCacheStats(std::shared_ptr<core::internal::PersistentStringCacheStats> const& p) noexcept;

    // We store a shared_ptr for efficiency. When the caller
    // retrieves the stats, we set p_ to point at the PersistentStringCachePersistentCacheStats
    // inside the cache. If the caller makes a copy or assigns,
    // We create a new instance, to provide value semantics. This means
    // that we don't have to copy all of the stats each time the caller
    // gets them.
    std::shared_ptr<internal::PersistentStringCacheStats const> p_;
    bool internal_;  // True if p_ points at the internal instance.

    // @cond
    friend class internal::PersistentStringCacheImpl;  // For access to constructor
    // @endcond
};

}  // namespace core
