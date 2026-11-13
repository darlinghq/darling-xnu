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

#include <stdint.h>
#include <sys/errno.h>

// process_policy (323): used by libsystem's proc_set_cpumon_params(),
// proc_disable_wakemon() and friends. Darling does not enforce CPU or
// wakeup monitors, background states or app types. Setting a policy is
// accepted as XNU accepts a valid request; a GET has nothing to report
// and fails with EINVAL, as XNU does for a policy with no state.
long sys_process_policy(int scope, int action, int policy, int subtype, void* attrp, int pid,
	unsigned long long tid)
{
	(void)scope;
	(void)policy;
	(void)subtype;
	(void)attrp;
	(void)pid;
	(void)tid;

	if (action < 1 || action > 8)
		return -EINVAL;
	if (action == 4 /* PROC_POLICY_ACTION_GET */)
		return -EINVAL;
	return 0;
}
