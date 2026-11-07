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

#include <darling/emulation/linux_premigration/ext/for-sandbox.h>

#include <sys/errno.h>

#include <darling/emulation/conversion/errno.h>

#include <darlingserver/rpc.h>
#include <darlingserver/rpc-supplement.h>

// darlingserver calls return 0 or a negative Linux errno value
static int to_bsd_error(int result) {
	return (result < 0) ? -errno_linux_to_bsd(result) : 0;
};

int __darling_sandbox_mark_applied(void) {
	return to_bsd_error(dserver_rpc_cs_mark_sandboxed());
};

int __darling_sandbox_is_applied(int32_t pid, bool* applied) {
	dserver_cs_state_t state;
	int error;

	// darlingserver takes 0 to mean the caller; here it is the kernel, which is no process of ours
	if (pid <= 0)
		return ESRCH;

	error = to_bsd_error(dserver_rpc_cs_get_state(pid, &state, sizeof(state)));
	if (error == 0)
		*applied = (state.darling_flags & DSERVER_CS_SANDBOXED) != 0;
	return error;
};
