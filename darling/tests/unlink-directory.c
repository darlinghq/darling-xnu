// unlink() of a directory fails with EPERM (xnu bsd/vfs/vfs_subr.c vn_authorize_unlink), not Linux's EISDIR, and leaves
// the directory alone; rmdir() and unlink() of a file still work. Foundation's item removal tells a directory from a
// file by that error, and a non-atomic NSFileWrapper write over an existing directory failed without it.
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

static void expect(int condition, const char* what)
{
	if (!condition) {
		printf("FAIL: %s (errno %d)\n", what, errno);
		exit(1);
	}
}

int main(void)
{
	char dir[] = "/tmp/unlink-directory-XXXXXX";
	expect(mkdtemp(dir) != NULL, "scratch directory");
	char sub[256], file[256];
	snprintf(sub, sizeof sub, "%s/sub", dir);
	snprintf(file, sizeof file, "%s/file", dir);
	expect(mkdir(sub, 0755) == 0, "mkdir");
	int fd = open(file, O_CREAT | O_WRONLY, 0644);
	expect(fd >= 0, "create file");
	close(fd);

	errno = 0;
	expect(unlink(sub) == -1 && errno == EPERM, "unlink of a directory is EPERM");
	struct stat info;
	expect(stat(sub, &info) == 0 && S_ISDIR(info.st_mode), "the directory is still there");
	errno = 0;
	expect(unlinkat(AT_FDCWD, sub, 0) == -1 && errno == EPERM, "unlinkat of a directory without AT_REMOVEDIR is EPERM");
	errno = 0;
	expect(unlink("/tmp/unlink-directory-does-not-exist") == -1 && errno == ENOENT, "unlink of a missing path is ENOENT");
	expect(unlink(file) == 0, "unlink of a file");
	expect(unlinkat(AT_FDCWD, sub, AT_REMOVEDIR) == 0, "unlinkat with AT_REMOVEDIR removes the directory");
	expect(rmdir(dir) == 0, "rmdir");
	printf("PASS: unlink of a directory is EPERM and changes nothing\n");
	return 0;
}
