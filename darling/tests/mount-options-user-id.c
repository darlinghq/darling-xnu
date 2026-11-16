// Host-side unit test for the parser that finds the owner of a FUSE mount in its /proc/self/mounts options, which
// unmount() uses to decide whether the caller may unmount the volume through the host's fusermount.
//
// Build and run from darling/ of the xnu repository (the include directory is made reachable as darling/emulation,
// as the SDK does):
//   E=src/libsystem_kernel/emulation; I=$(mktemp -d); mkdir $I/darling; ln -s $PWD/$E/include $I/darling/emulation
//   cc -std=gnu11 -Wall -Wextra -Werror -I$I tests/mount-options-user-id.c $E/src/xnu_syscall/bsd/helper/stat/mount_options.c
//      -o $I/test && $I/test
#include <stdio.h>

#include <darling/emulation/xnu_syscall/bsd/helper/stat/mount_options.h>

static int failures;

static void check(const char* options, int expected_found, unsigned int expected_uid)
{
	unsigned int uid = 12345;
	int found = mount_options_fuse_user_id(options, &uid);

	if (found == expected_found && (!found || uid == expected_uid))
		printf("PASS: \"%s\"\n", options);
	else {
		printf("FAIL: \"%s\" (found %d uid %u, expected found %d uid %u)\n", options, found, uid, expected_found,
			expected_uid);
		failures++;
	}
}

int main(void)
{
	check("rw,nosuid,nodev,relatime,user_id=1000,group_id=1000", 1, 1000);
	check("rw,user_id=0", 1, 0);
	check("user_id=4294967295", 1, 4294967295u);
	check("rw,nosuid", 0, 0);
	check("rw,xuser_id=5", 0, 0);
	check("", 0, 0);
	check("user_id=", 0, 0);
	check("user_id=12a", 0, 0);
	check("user_id=4294967296", 0, 0);
	return failures ? 1 : 0;
}
