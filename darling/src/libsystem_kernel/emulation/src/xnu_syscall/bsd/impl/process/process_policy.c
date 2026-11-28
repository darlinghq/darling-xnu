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
#include <libsimple/lock.h>

#include <darling/emulation/xnu_syscall/bsd/impl/signal/kill.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/getpid.h>

// process_policy (323), <sys/process_policy.h>: libproc's proc_set_cpumon_params(), proc_get_cpumon_params(),
// proc_disable_cpumon() and friends call it with PROC_POLICY_SCOPE_PROCESS, PROC_POLICY_RESOURCE_USAGE and
// PROC_POLICY_RUSAGE_CPU. Darling does not enforce CPU monitors, background states or app types: a change is
// accepted for an existing process, and the CPU monitor parameters set for the calling process are remembered, so
// proc_get_cpumon_params() answers what was set (and all zero before, as macOS does).

#define PROC_POLICY_SCOPE_PROCESS 1
#define PROC_POLICY_SCOPE_THREAD 2
#define PROC_POLICY_ACTION_SET 10
#define PROC_POLICY_ACTION_GET 11
#define PROC_POLICY_ACTION_FIRST 1
#define PROC_POLICY_ACTION_LAST 15
#define PROC_POLICY_RESOURCE_USAGE 4
#define PROC_POLICY_RUSAGE_CPU 3

struct policy_cpuusage_attr // proc_policy_cpuusage_attr_t
{
	uint32_t cpu_attr;
	uint32_t cpu_percentage;
	uint64_t interval;
	uint64_t deadline;
};

static struct policy_cpuusage_attr cpumon; // all zero: no monitor
static libsimple_lock_t cpumon_lock = LIBSIMPLE_LOCK_INITIALIZER;

long sys_process_policy(int scope, int action, int policy, int subtype, void* attrp, int pid,
	unsigned long long tid)
{
	(void)tid;

	if (scope != PROC_POLICY_SCOPE_PROCESS && scope != PROC_POLICY_SCOPE_THREAD)
		return -EINVAL;
	if (action < PROC_POLICY_ACTION_FIRST || action > PROC_POLICY_ACTION_LAST)
		return -EINVAL;

	// the target must exist (ESRCH), signal 0 only checks that
	int self = pid == 0 || pid == (int) sys_getpid();
	if (!self)
	{
		long r = sys_kill(pid, 0, 1);
		if (r < 0 && r != -EPERM)
			return r;
	}

	if (scope == PROC_POLICY_SCOPE_PROCESS && policy == PROC_POLICY_RESOURCE_USAGE && subtype == PROC_POLICY_RUSAGE_CPU)
	{
		if (action == PROC_POLICY_ACTION_SET)
		{
			if (attrp == NULL)
				return -EFAULT;
			if (self)
			{
				libsimple_lock_lock(&cpumon_lock);
				cpumon = *(const struct policy_cpuusage_attr*) attrp;
				libsimple_lock_unlock(&cpumon_lock);
			}
			return 0;
		}
		if (action == PROC_POLICY_ACTION_GET)
		{
			struct policy_cpuusage_attr cur = { 0 };
			if (attrp == NULL)
				return -EFAULT;
			if (self)
			{
				libsimple_lock_lock(&cpumon_lock);
				cur = cpumon;
				libsimple_lock_unlock(&cpumon_lock);
			}
			*(struct policy_cpuusage_attr*) attrp = cur;
			return 0;
		}
	}

	// every other policy is accepted and has no effect; there is no state to report for a GET
	if (action == PROC_POLICY_ACTION_GET)
		return -EINVAL;
	return 0;
}
