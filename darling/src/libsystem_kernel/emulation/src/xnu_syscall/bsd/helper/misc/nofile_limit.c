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

// The guest's RLIMIT_NOFILE.
//
// Besides the guest's descriptors, a Darling process holds descriptors that
// belong to Darling itself: each thread's darlingserver RPC socket, which
// mldr places just below the Linux soft limit the process started with (see
// mldr's socket_bitmap). Linux refuses to create a descriptor at or above its
// soft limit, and nothing can move one there either, so once the Linux limit
// is lowered, threads created afterwards (and the child of a fork) can't get
// a socket. A Darwin kernel doesn't count its own resources against the
// process's descriptor limit, so lowering RLIMIT_NOFILE must not affect them.
//
// Therefore the guest's setrlimit never lowers the Linux limit. Once the guest
// sets RLIMIT_NOFILE, its (Darwin) values are kept here, getrlimit and
// getdtablesize report them, and the calls that create descriptors enforce the
// soft limit with Darwin's errors (EMFILE, or EBADF/EINVAL for dup2/F_DUPFD
// targets). execve passes the values on to mldr, which hands them back to the
// new image (elfcalls inherited_nofile_limit). Until the guest sets a limit,
// the Linux limit applies as before and is reported capped at OPEN_MAX.

#include <darling/emulation/xnu_syscall/bsd/helper/misc/nofile_limit.h>

#include <sys/errno.h>
#include <stddef.h>
#include <libsimple/lock.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/common/simple.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/close.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/geteuid.h>

#include <elfcalls.h>

extern struct elf_calls* _elfcalls;

enum {
	// not looked up yet
	nofile_state_unknown,
	// the guest has not set a limit; the Linux limit applies
	nofile_state_host,
	// the guest's limit is in nofile_cur/nofile_max
	nofile_state_guest,
};

static libsimple_lock_t nofile_lock = LIBSIMPLE_LOCK_INITIALIZER;
static int nofile_state = nofile_state_unknown;
static unsigned int nofile_cur, nofile_max;

// The soft limit enforced on new descriptors, or -1 for none (Linux's own).
static int nofile_enforced = -1;

static unsigned int nofile_clamp(unsigned long long value)
{
	return value > DARWIN_OPEN_MAX ? DARWIN_OPEN_MAX : (unsigned int)value;
}

static void nofile_store_locked(unsigned int cur, unsigned int max)
{
	nofile_cur = cur;
	nofile_max = max;
	// A soft limit of OPEN_MAX stands for any value of OPEN_MAX or more
	// (see the QUIRK in nofile_limit_set), so it is not enforced here; the
	// Linux soft limit, which is at least that high, applies.
	__atomic_store_n(&nofile_enforced, cur < DARWIN_OPEN_MAX ? (int)cur : -1, __ATOMIC_RELAXED);
	__atomic_store_n(&nofile_state, nofile_state_guest, __ATOMIC_RELEASE);
}

static void nofile_init_locked(void)
{
	unsigned int inherited[2];

	if (nofile_state != nofile_state_unknown)
		return;

	if (_elfcalls == NULL)
		return; // too early to tell; try again on the next call

	if (_elfcalls->inherited_nofile_limit(inherited) == 0 && inherited[0] <= inherited[1])
		nofile_store_locked(nofile_clamp(inherited[0]), nofile_clamp(inherited[1]));
	else
		__atomic_store_n(&nofile_state, nofile_state_host, __ATOMIC_RELEASE);
}

static void nofile_init(void)
{
	if (__atomic_load_n(&nofile_state, __ATOMIC_ACQUIRE) != nofile_state_unknown)
		return;

	libsimple_lock_lock(&nofile_lock);
	nofile_init_locked();
	libsimple_lock_unlock(&nofile_lock);
}

static long host_nofile(struct xnu_rlimit* lim)
{
	int ret = LINUX_SYSCALL(__NR_prlimit64, 0, LINUX_RLIMIT_NOFILE, 0, lim);
	if (ret < 0)
		return errno_linux_to_bsd(ret);
	return 0;
}

// The guest's current limit, clamped to OPEN_MAX.
static long nofile_current_locked(unsigned int* cur, unsigned int* max)
{
	struct xnu_rlimit host;
	long ret;

	nofile_init_locked();

	if (nofile_state == nofile_state_guest) {
		*cur = nofile_cur;
		*max = nofile_max;
		return 0;
	}

	ret = host_nofile(&host);
	if (ret < 0)
		return ret;

	*cur = nofile_clamp(host.rlim_cur);
	*max = nofile_clamp(host.rlim_max);
	return 0;
}

long nofile_limit_get(struct xnu_rlimit* rlp)
{
	unsigned int cur, max;
	long ret;

	libsimple_lock_lock(&nofile_lock);
	ret = nofile_current_locked(&cur, &max);
	libsimple_lock_unlock(&nofile_lock);

	if (ret < 0)
		return ret;

	rlp->rlim_cur = cur;
	rlp->rlim_max = max;
	return 0;
}

// QUIRK: setrlimit(RLIMIT_NOFILE) accepts RLIM_INFINITY as the soft limit.
// The setrlimit(2) man page (COMPATIBILITY) says it fails with EINVAL, but
// black-box runs on macOS 27.0.1 arm64 (tier 2) return 0 for
// {RLIM_INFINITY, RLIM_INFINITY}, {OPEN_MAX, RLIM_INFINITY} and soft limits
// above OPEN_MAX while the hard limit is the default RLIM_INFINITY. They
// return EINVAL only when the soft limit exceeds the requested hard limit, and
// EPERM (as non-root) for a hard limit of RLIM_INFINITY once the hard limit
// has been lowered.
// getrlimit never reports more than OPEN_MAX, so every value of OPEN_MAX or
// more is taken as OPEN_MAX: with an unlowered hard limit (reported as
// OPEN_MAX when the host's is at least that), RLIM_INFINITY is accepted, and
// once the hard limit is lowered, it is a raise that needs privilege.
// Tests: rlimit_edge_cases.c, test_quirk_nofile_infinity();
// tests/rlimit-infinity.rb.
long nofile_limit_set(const struct xnu_rlimit* rlp)
{
	unsigned int cur = nofile_clamp(rlp->rlim_cur);
	unsigned int max = nofile_clamp(rlp->rlim_max);
	unsigned int old_cur, old_max;
	struct xnu_rlimit host, wanted;
	long ret;

	// the caller has checked that the soft limit is not above the hard one
	libsimple_lock_lock(&nofile_lock);

	ret = nofile_current_locked(&old_cur, &old_max);
	if (ret < 0)
		goto out;

	// Raising the hard limit needs privilege (setrlimit(2) EPERM). Linux
	// can't tell: its own hard limit is never lowered by the guest.
	if (max > old_max && sys_geteuid() != 0) {
		ret = -EPERM;
		goto out;
	}

	ret = host_nofile(&host);
	if (ret < 0)
		goto out;

	// Raise the Linux limit where the new one needs more; never lower it.
	wanted = host;
	if (max > wanted.rlim_max)
		wanted.rlim_max = max;
	if (cur > wanted.rlim_cur)
		wanted.rlim_cur = cur;

	if (wanted.rlim_cur != host.rlim_cur || wanted.rlim_max != host.rlim_max) {
		int err = LINUX_SYSCALL(__NR_prlimit64, 0, LINUX_RLIMIT_NOFILE, &wanted, 0);
		if (err < 0) {
			ret = errno_linux_to_bsd(err);
			goto out;
		}
	}

	nofile_store_locked(cur, max);
	ret = 0;

out:
	libsimple_lock_unlock(&nofile_lock);
	return ret;
}

int nofile_limit_exceeded(int fd)
{
	int limit;

	nofile_init();
	limit = __atomic_load_n(&nofile_enforced, __ATOMIC_RELAXED);
	return limit >= 0 && fd >= limit;
}

long nofile_limit_check_new(long fd)
{
	if (fd < 0 || !nofile_limit_exceeded(fd))
		return fd;

	close_internal(fd);
	return -EMFILE;
}

int nofile_limit_exec_env(char* buf, unsigned long size)
{
	int have = 0;

	libsimple_lock_lock(&nofile_lock);
	nofile_init_locked();
	if (nofile_state == nofile_state_guest) {
		__simple_snprintf(buf, size, "__mldr_nofile=%u,%u", nofile_cur, nofile_max);
		have = 1;
	}
	libsimple_lock_unlock(&nofile_lock);

	return have;
}

void nofile_limit_postfork_child(void)
{
	// the forking thread may not be the one that held the lock
	libsimple_lock_init(&nofile_lock);
}
