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
\brief Indicates the discard policy to make room for entries when the cache is full.

Once the cache is full and another entry is added,
`LRU_TTL` unconditionally deletes all entries that have expired and then,
if deleting these entries did not create sufficient free space, deletes entries
in LRU order until enough space is available.

If the discard policy is set to `LRU_only`, entries do not maintain an expiry time and
are therefore discarded strictly in LRU order.
*/
enum class CacheDiscardPolicy
{
    LRU_TTL,
    LRU_only
};

}  // namespace core
