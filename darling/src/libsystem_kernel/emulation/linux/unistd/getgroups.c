#include "getgroups.h"
#include "../base.h"
#include "../errno.h"
#include <linux-syscalls/linux.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <libsimple/lock.h>
#include <darlingserver/rpc.h>
#include <sys/syslimits.h>
#include "../simple.h"
#include "../duct_errno.h"

// we use the same approach as {g,s}et{u,g}id

static libsimple_rwlock_t groups_rwlock = LIBSIMPLE_RWLOCK_INITIALIZER;
static gid_t groups[NGROUPS_MAX] = { [0 ... (NGROUPS_MAX - 1)] = -1 };

// make sure the groups array doesn't become too large to be allocated on the stack.
// the current value of `NGROUPS_MAX` is 16, so the groups array is 64 bytes wide.
// this assertion is here merely in case this value changes in the future (it's not unheard of;
// Linux increased the value from 32 to 65536, so Darwin is free to do the same).
_Static_assert(sizeof(groups) <= 256, "Groups array is too large to reasonably fit on the stack");

// darlingserver assumes `gid_t` == `uint32_t`
_Static_assert(sizeof(gid_t) == sizeof(uint32_t), "sizeof(gid_t) != sizeof(uint32_t)");

long __getgroups(unsigned int size, unsigned int* gidset) {
	gid_t groups_copy[NGROUPS_MAX];
	long group_count = 0;

	libsimple_rwlock_lock_read(&groups_rwlock);
	memcpy(groups_copy, groups, sizeof(groups_copy));
	libsimple_rwlock_unlock_read(&groups_rwlock);

	// if it's uninitialized, initialize it
	if (groups_copy[0] == -1) {
		libsimple_rwlock_lock_write(&groups_rwlock);
		// check again if it's still uninitialized
		if (groups[0] == -1) {
			// fetch them from the server
			memset(groups, 0xff, sizeof(groups)); // effectively fills the array with `-1`
			if (dserver_rpc_groups(NULL, 0, groups, sizeof(groups) / sizeof(*groups), NULL) < 0) {
				__simple_abort();
			}
		}
		memcpy(groups_copy, groups, sizeof(groups_copy));
		libsimple_rwlock_unlock_write(&groups_rwlock);
	}

	// count up how many groups we have
	for (; group_count < sizeof(groups_copy) / sizeof(*groups_copy); ++group_count) {
		if (groups_copy[group_count] == -1) {
			break;
		}
	}

	if (size > 0) {
		if (size < group_count) {
			return -EINVAL;
		}

		memcpy(gidset, groups_copy, group_count * sizeof(gid_t));
	}

	return group_count;
};

long __setgroups(unsigned int size, const unsigned int* gidset) {
	if (size > NGROUPS_MAX) {
		return -EINVAL;
	}

	libsimple_rwlock_lock_write(&groups_rwlock);
	if (dserver_rpc_groups(gidset, size, NULL, 0, NULL) < 0) {
		__simple_abort();
	}
	memset(groups, 0xff, sizeof(groups)); // effectively fills the array with `-1`
	memcpy(groups, gidset, size * sizeof(gid_t));
	libsimple_rwlock_unlock_write(&groups_rwlock);

	return 0;
};

long sys_getgroups(unsigned int size, int* gidset)
{
	// NOTE(facekapow): is this actually used anywhere?
	//                  this seems like a vestige of the old UID/GID system (before we were faking it).
	if (getenv("__FAKE_SETGID_ROOT"))
	{
		if (size >= 1)
		{
			gidset[0] = 0;
			return 1;
		}
	}

	return __getgroups(size, (unsigned int*)gidset);
}

