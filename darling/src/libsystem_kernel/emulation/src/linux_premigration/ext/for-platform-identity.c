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

#include <darling/emulation/linux_premigration/ext/for-platform-identity.h>

#include <sys/errno.h>

#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/xnu_syscall/bsd/helper/misc/platform_identity.h>

#include <darlingserver/rpc.h>

int __darling_get_platform_identity(dserver_platform_identity_t* identity) {
	const dserver_platform_identity_t* cached = platform_identity();
	if (!cached)
		return EIO;
	__builtin_memcpy(identity, cached, sizeof(*identity));
	return 0;
};

int __darling_set_platform_identity(const dserver_platform_identity_t* identity) {
	// darlingserver calls return 0 or a negative Linux errno value
	int result = dserver_rpc_set_platform_identity(identity, sizeof(*identity));
	return (result < 0) ? -errno_linux_to_bsd(result) : 0;
};
