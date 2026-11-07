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

#include <darling/emulation/xnu_syscall/bsd/helper/misc/platform_identity.h>

#include <stddef.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

#include <darlingserver/rpc.h>

enum {
	identity_empty,
	identity_filling,
	identity_ready,
};

static int identity_state = identity_empty;
static dserver_platform_identity_t identity;

const dserver_platform_identity_t* platform_identity(void)
{
	if (__atomic_load_n(&identity_state, __ATOMIC_ACQUIRE) == identity_ready)
		return &identity;

	dserver_platform_identity_t fetched;
	if (dserver_rpc_get_platform_identity(&fetched, sizeof(fetched)) < 0)
		return NULL;

	// the first thread to get here publishes what it fetched; the others use that copy,
	// so the whole process sees one identity even if a new one was pushed in between
	int expected = identity_empty;
	if (__atomic_compare_exchange_n(&identity_state, &expected, identity_filling, 0, __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE))
	{
		__builtin_memcpy(&identity, &fetched, sizeof(identity));
		__atomic_store_n(&identity_state, identity_ready, __ATOMIC_RELEASE);
	}
	else
	{
		while (__atomic_load_n(&identity_state, __ATOMIC_ACQUIRE) != identity_ready)
			LINUX_SYSCALL0(__NR_sched_yield);
	}

	return &identity;
}
