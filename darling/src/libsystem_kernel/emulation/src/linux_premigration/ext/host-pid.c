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

#include <darling/emulation/linux_premigration/ext/host-pid.h>

#include <darling/emulation/conversion/errno.h>

#include <darlingserver/rpc.h>

// darlingserver calls return 0 or a negative Linux errno value
static int to_bsd_error(int result) {
	return (result < 0) ? -errno_linux_to_bsd(result) : 0;
};

int __darling_host_pid(pid_t* host_pid) {
	int32_t value = 0;
	int result = to_bsd_error(dserver_rpc_host_pid(&value));

	if (result == 0)
		*host_pid = value;
	return result;
};

int __darling_pid_for_host_pid(pid_t host_pid, pid_t* pid) {
	int32_t value = 0;
	int result = to_bsd_error(dserver_rpc_pid_for_host_pid(host_pid, &value));

	if (result == 0)
		*pid = value;
	return result;
};
