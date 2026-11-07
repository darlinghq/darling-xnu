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

#ifndef _DARLING_EMULATION_FOR_SANDBOX_H_
#define _DARLING_EMULATION_FOR_SANDBOX_H_

#include <darling/emulation/common/base.h>
#include <stdbool.h>
#include <stdint.h>

CPP_EXTERN_BEGIN

// Darling-private access to the sandbox state that darlingserver keeps for
// each process, for libsystem_sandbox. Darling enforces no sandbox; the state
// only records that a profile was applied. Each returns 0 or a BSD errno value.

/**
 * Records that the calling process applied a sandbox profile. This cannot be
 * undone; forked children and executed images stay sandboxed.
 */
VISIBLE
int __darling_sandbox_mark_applied(void);

/**
 * Sets @p applied to whether the process with ID @p pid, or one of the
 * processes it descends from, applied a sandbox profile. Fails with `ESRCH`
 * when there is no such process (including any `pid <= 0`).
 */
VISIBLE
int __darling_sandbox_is_applied(int32_t pid, bool* applied);

CPP_EXTERN_END

#endif // _DARLING_EMULATION_FOR_SANDBOX_H_
