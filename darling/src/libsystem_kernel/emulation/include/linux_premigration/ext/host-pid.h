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

#ifndef _DARLING_EMULATION_HOST_PID_H_
#define _DARLING_EMULATION_HOST_PID_H_

#include <sys/types.h>

#include <darling/emulation/common/base.h>

CPP_EXTERN_BEGIN

// Darling-private translation between the PIDs Darling processes see and
// the PIDs the host sees, for protocols shared with host programs (such as
// EWMH's _NET_WM_PID). Each returns 0 or a BSD errno value.

/**
 * Stores the calling process's PID in the host's PID namespace in
 * @p host_pid.
 */
VISIBLE
int __darling_host_pid(pid_t* host_pid);

/**
 * Stores the PID of the Darling process whose host PID is @p host_pid in
 * @p pid. Fails with `ESRCH` when no Darling process has that host PID.
 */
VISIBLE
int __darling_pid_for_host_pid(pid_t host_pid, pid_t* pid);

CPP_EXTERN_END

#endif // _DARLING_EMULATION_HOST_PID_H_
