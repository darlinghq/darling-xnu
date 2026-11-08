/**
 * This file is part of Darling.
 *
 * Copyright (C) 2026 Darling Developers
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

#include <darling/emulation/xnu_syscall/bsd/impl/misc/mac_syscall.h>

#include <stddef.h>
#include <string.h>
#include <sys/errno.h>

// the longest MAC policy name, NUL included (MAC_MAX_POLICY_NAME in
// <security/mac.h>)
#define POLICY_NAME_MAX 32

// __mac_syscall() hands a call to the MAC policy module with the given
// name. Darling has no policy modules, but programs expect the two that
// every macOS system registers, Sandbox and AMFI. Neither implements any
// call here: Darling's Sandbox emulation lives in libsystem_sandbox and
// enforces nothing, and AMFI's calls are not documented. Any other name is
// not a registered policy (ENOPOLICY, as documented in <sys/errno.h>).
long sys___mac_syscall(const char* policy, int call, void* arg)
{
	char name[POLICY_NAME_MAX];
	size_t i;

	if (policy == NULL)
		return -EFAULT;

	for (i = 0; i < sizeof(name); i++) {
		name[i] = policy[i];
		if (name[i] == '\0')
			break;
	}
	if (i == sizeof(name))
		return -ENAMETOOLONG;

	// QUIRK: for a call they do not support, the Sandbox policy fails with
	// ENOTSUP and the AMFI policy with ENOSYS. Undocumented. Evidence:
	// black-box observation on macOS 27.0.1, arm64 and x86_64 (tier 2):
	// __mac_syscall("Sandbox", 9999, NULL) gives ENOTSUP,
	// __mac_syscall("AMFI", 9999, NULL) gives ENOSYS, and an unknown name
	// gives ENOPOLICY. Test: test_quirk_mac_syscall_unsupported_call in
	// mac_syscall.c (P86 tests).
	if (strcmp(name, "Sandbox") == 0)
		return -ENOTSUP;
	if (strcmp(name, "AMFI") == 0)
		return -ENOSYS;

	return -ENOPOLICY;
}
