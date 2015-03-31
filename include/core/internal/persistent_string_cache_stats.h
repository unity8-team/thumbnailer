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
#include <core/persistent_cache_stats.h>

namespace core
{

namespace internal
{

// Simple stats class to keep track of accesses.

struct PersistentStringCacheStats
{
    PersistentStringCacheStats() noexcept
        : policy_(CacheDiscardPolicy::LRU_only)
        , num_entries_(0)
        , cache_size_(0)
        , max_cache_size_(0)
        , headroom_(0)
        , state_(Initialized)
    {
        clear();
    }

    PersistentStringCacheStats(PersistentStringCacheStats const&) = default;
    PersistentStringCacheStats(PersistentStringCacheStats&&) = default;
    PersistentStringCacheStats& operator=(PersistentStringCacheStats const&) = default;
    PersistentStringCacheStats& operator=(PersistentStringCacheStats&&) = default;

    std::string cache_path_;           // Immutable
    core::CacheDiscardPolicy policy_;  // Immutable
    int64_t num_entries_;
    int64_t cache_size_;
    int64_t max_cache_size_;
    int64_t headroom_;

    // Values below are reset by a call to clear().
    int64_t hits_;
    int64_t misses_;
    int64_t hits_since_last_miss_;
    int64_t misses_since_last_hit_;
    int64_t longest_hit_run_;
    int64_t longest_miss_run_;
    std::chrono::steady_clock::time_point most_recent_hit_time_;
    std::chrono::steady_clock::time_point most_recent_miss_time_;
    std::chrono::steady_clock::time_point longest_hit_run_time_;
    std::chrono::steady_clock::time_point longest_miss_run_time_;

    enum State
    {
        Initialized,
        LastAccessWasHit,
        LastAccessWasMiss
    };
    State state_;

    void inc_hits()
    {
        ++hits_since_last_miss_;
        ++hits_;
        most_recent_hit_time_ = std::chrono::steady_clock::now();
        misses_since_last_hit_ = 0;
        if (state_ != LastAccessWasHit)
        {
            state_ = LastAccessWasHit;
            misses_since_last_hit_ = 0;
        }
        if (state_ != LastAccessWasMiss && hits_since_last_miss_ > longest_hit_run_)
        {
            longest_hit_run_ = hits_since_last_miss_;
            longest_hit_run_time_ = most_recent_hit_time_;
        }
    }

    void inc_misses()
    {
        ++misses_since_last_hit_;
        ++misses_;
        most_recent_miss_time_ = std::chrono::steady_clock::now();
        hits_since_last_miss_ = 0;
        if (state_ != LastAccessWasMiss)
        {
            state_ = LastAccessWasMiss;
            hits_since_last_miss_ = 0;
        }
        if (state_ != LastAccessWasHit && misses_since_last_hit_ > longest_miss_run_)
        {
            longest_miss_run_ = misses_since_last_hit_;
            longest_miss_run_time_ = most_recent_miss_time_;
        }
    }

    void clear() noexcept
    {
        hits_ = 0;
        misses_ = 0;
        hits_since_last_miss_ = 0;
        misses_since_last_hit_ = 0;
        longest_hit_run_ = 0;
        longest_miss_run_ = 0;
        most_recent_hit_time_ = std::chrono::steady_clock::time_point();
        most_recent_miss_time_ = std::chrono::steady_clock::time_point();
        longest_hit_run_time_ = std::chrono::steady_clock::time_point();
        longest_miss_run_time_ = std::chrono::steady_clock::time_point();
    }
};

}  // namespace internal

}  // namespace core
