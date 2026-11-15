// fsctl() and ffsctl() must fail the way xnu does for a file system without the command (ENOTTY), and still
// report a missing path (ENOENT) or a bad descriptor (EBADF), instead of being an unimplemented system call.
// Apple's item replacement directory code (NSFileManager) issues one such command (_IOWR('h', 47, uint32_t)).
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

extern int fsctl(const char* path, unsigned long request, void* data, unsigned int options);
extern int ffsctl(int fd, unsigned long request, void* data, unsigned int options);

#define HFS_COMMAND 0xC004682FUL // _IOWR('h', 47, uint32_t)
#define FSIOC_FD_ONLY_OPEN_ONCE 0xC0044115UL // _IOWR('A', 21, uint32_t)

static void expect(int condition, const char* what)
{
	if (!condition) {
		printf("FAIL: %s (errno %d)\n", what, errno);
		exit(1);
	}
}

int main(void)
{
	char path[] = "/tmp/fsctl-unsupported-XXXXXX";
	int fd = mkstemp(path);
	expect(fd >= 0, "scratch file");
	unsigned int value = 0;

	errno = 0;
	expect(fsctl(path, HFS_COMMAND, &value, 0) == -1 && errno == ENOTTY, "fsctl with a file system command is ENOTTY");
	errno = 0;
	expect(fsctl("/nonexistent-fsctl-path", HFS_COMMAND, &value, 0) == -1 && errno == ENOENT, "fsctl on a missing path is ENOENT");
	errno = 0;
	expect(fsctl(path, FSIOC_FD_ONLY_OPEN_ONCE, &value, 0) == -1 && errno == EINVAL, "fsctl cannot ask whether the file is open only once");
	errno = 0;
	expect(ffsctl(fd, HFS_COMMAND, &value, 0) == -1 && errno == ENOTTY, "ffsctl with a file system command is ENOTTY");
	errno = 0;
	expect(ffsctl(-1, HFS_COMMAND, &value, 0) == -1 && errno == EBADF, "ffsctl on a bad descriptor is EBADF");

	// FSOPT_NOFOLLOW: a dangling symbolic link is a missing path when followed and an existing item when not
	char link[] = "/tmp/fsctl-unsupported-link-XXXXXX";
	close(mkstemp(link));
	unlink(link);
	expect(symlink("/nonexistent-fsctl-target", link) == 0, "dangling symbolic link");
	errno = 0;
	expect(fsctl(link, HFS_COMMAND, &value, 0) == -1 && errno == ENOENT, "fsctl follows a symbolic link");
	errno = 0;
	expect(fsctl(link, HFS_COMMAND, &value, 1 /* FSOPT_NOFOLLOW */) == -1 && errno == ENOTTY, "fsctl with FSOPT_NOFOLLOW does not");
	unlink(link);

	close(fd);
	unlink(path);
	printf("PASS: fsctl and ffsctl fail like a file system without the command\n");
	return 0;
}
