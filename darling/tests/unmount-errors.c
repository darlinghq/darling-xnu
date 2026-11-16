// unmount(2) must report the errors its manual page and macOS give for paths that are not mount points,
// missing paths and bad pointers, instead of failing with EBUSY for everything.
// Every value expected for a non-root user was confirmed by running this program on macOS 26.7.3 (25G342), x86_64.
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/param.h>
#include <unistd.h>

static int failures;

static void expect_error_flags(const char* what, const char* path, int flags, int expected)
{
	errno = 0;
	int rv = unmount(path, flags);
	if (rv == -1 && errno == expected)
		printf("PASS: %s\n", what);
	else {
		printf("FAIL: %s (rv %d, errno %d %s, expected errno %d %s)\n", what, rv, errno, strerror(errno),
			expected, strerror(expected));
		failures++;
	}
}

static void expect_error(const char* what, const char* path, int expected)
{
	expect_error_flags(what, path, 0, expected);
}

int main(void)
{
	char dir[] = "/tmp/unmount-errors-XXXXXX";
	if (!mkdtemp(dir)) {
		printf("FAIL: mkdtemp\n");
		return 1;
	}

	char file[MAXPATHLEN], link[MAXPATHLEN];
	snprintf(file, sizeof(file), "%s/file", dir);
	snprintf(link, sizeof(link), "%s-link", dir);
	int fd = open(file, O_CREAT | O_WRONLY, 0644);
	if (fd < 0 || symlink(dir, link) != 0) {
		printf("FAIL: scratch files\n");
		return 1;
	}
	close(fd);

	expect_error("directory that is not a mount point", dir, EINVAL);
	expect_error("missing path", "/nonexistent-unmount-test", ENOENT);
	expect_error("NULL path", NULL, EFAULT);
	expect_error("regular file", file, EINVAL);
	expect_error("symlink to a directory that is not a mount point", link, EINVAL);
	if (geteuid() != 0) {
		expect_error("root file system as a non-root user", "/", EPERM);
		expect_error_flags("root file system, forced, as a non-root user", "/", MNT_FORCE, EPERM);
	} else {
		// Not observed on macOS (no root there); the root volume is always in use, which unmount(2) reports as
		// EBUSY, also when forced.
		expect_error("root file system as root", "/", EBUSY);
		expect_error_flags("root file system, forced, as root", "/", MNT_FORCE, EBUSY);
	}

	unlink(link);
	unlink(file);
	rmdir(dir);
	return failures ? 1 : 0;
}
