/**
 * This file is part of Darling.
 *
 * Copyright (C) 2026 Lubos Dolezel
 *
 * Darling is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Darling is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Darling.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _DARLING_EMULATION_FOR_IOSURFACE_H_
#define _DARLING_EMULATION_FOR_IOSURFACE_H_

#include <stdint.h>

#include <darling/emulation/common/base.h>

#include <darlingserver/rpc-supplement.h>

CPP_EXTERN_BEGIN

// Darling-private access to darlingserver's IOSurface service, for IOSurface.framework (see
// "IOSurface service" in darlingserver's rpc-supplement.h). Each returns 0 or a BSD errno
// value. Descriptors returned are new, closed on exec, and belong to the caller.

/**
 * Holds the surface whose shared memory object is @p memory for the calling process and stores
 * its ID in @p surface_id. @p id_hint, @p flags (`DSERVER_IOSURFACE_GLOBAL`) and
 * @p counter_offset apply when the service does not know the surface yet.
 */
VISIBLE
int __darling_iosurface_retain(int memory, uint32_t id_hint, uint32_t flags, uint32_t counter_offset, uint32_t* surface_id);

/**
 * Drops one hold of the calling process.
 */
VISIBLE
int __darling_iosurface_release(uint32_t surface_id);

/**
 * Stores a descriptor of the shared memory object of the surface in @p memory; only for global
 * surfaces and surfaces the caller holds (`ENOENT` otherwise).
 */
VISIBLE
int __darling_iosurface_lookup(uint32_t surface_id, int* memory);

/**
 * Offers @p buffer, described by @p info, as the surface's GPU buffer, and stores the buffer the
 * surface keeps (the first one offered) in @p published and its description in @p info.
 */
VISIBLE
int __darling_iosurface_publish_gpu_buffer(uint32_t surface_id, int buffer, dserver_iosurface_gpu_buffer_t* info, int* published);

/**
 * Stores the surface's GPU buffer in @p buffer and its description in @p info; `ENOENT` if it
 * has none.
 */
VISIBLE
int __darling_iosurface_copy_gpu_buffer(uint32_t surface_id, dserver_iosurface_gpu_buffer_t* info, int* buffer);

/**
 * Records how the calling process uses the surface (`DSERVER_IOSURFACE_USAGE_*`).
 */
VISIBLE
int __darling_iosurface_set_usage(uint32_t surface_id, uint32_t usage);

/**
 * The number of processes holding the surface, how many of them may read its memory, and
 * `DSERVER_IOSURFACE_STATE_*` flags; `ENOENT` once no process holds it.
 */
VISIBLE
int __darling_iosurface_get_state(uint32_t surface_id, uint32_t* holders, uint32_t* memory_users, uint32_t* flags);

CPP_EXTERN_END

#endif // _DARLING_EMULATION_FOR_IOSURFACE_H_
