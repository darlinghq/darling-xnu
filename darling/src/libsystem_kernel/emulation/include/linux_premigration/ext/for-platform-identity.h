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

#ifndef _DARLING_EMULATION_FOR_PLATFORM_IDENTITY_H_
#define _DARLING_EMULATION_FOR_PLATFORM_IDENTITY_H_

#include <darling/emulation/common/base.h>
#include <darlingserver/rpc-supplement.h>

CPP_EXTERN_BEGIN

// Darling-private access to the platform identity that darlingserver holds
// (see `dserver_platform_identity_t`). Each returns 0 or a BSD errno value.

/**
 * Copies the platform identity of this process into @p identity: the one
 * that `hw.model`, `machdep.cpu.brand_string` and `gethostuuid()` report.
 * It is read from darlingserver once per process.
 */
VISIBLE
int __darling_get_platform_identity(dserver_platform_identity_t* identity);

/**
 * Pushes a new platform identity, for processes started afterwards. Only
 * root may do this (`EPERM` otherwise); a malformed identity fails with
 * `EINVAL`.
 */
VISIBLE
int __darling_set_platform_identity(const dserver_platform_identity_t* identity);

CPP_EXTERN_END

#endif // _DARLING_EMULATION_FOR_PLATFORM_IDENTITY_H_
