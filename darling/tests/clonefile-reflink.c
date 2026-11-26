// clonefileat()/fclonefileat() must create a distinct file with the source's
// contents and metadata through a host reflink. EOPNOTSUPP is allowed when the
// host filesystem cannot reflink; ENOSYS and a plain-data copy are not.
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/clonefile.h>
#include <sys/stat.h>
#include <unistd.h>

static void expect(int condition, const char* what)
{
	if (!condition) {
		printf("FAIL: %s (errno %d)\n", what, errno);
		exit(1);
	}
}

static void expectUnchanged(const char* src, const char* dst)
{
	struct stat ss, ds;
	char buf[64] = { 0 };
	int fd;
	ssize_t got;

	expect(stat(src, &ss) == 0, "stat source");
	expect(stat(dst, &ds) == 0, "stat clone");
	expect(ss.st_ino != ds.st_ino, "clone is a different file");
	expect(ss.st_size == ds.st_size, "clone has the source size");
	expect((ss.st_mode & 07777) == (ds.st_mode & 07777), "clone has the source mode");
	expect(ss.st_mtime == ds.st_mtime, "clone has the source mtime");

	fd = open(dst, O_RDONLY);
	expect(fd >= 0, "open clone");
	got = read(fd, buf, sizeof(buf) - 1);
	expect(got == (ssize_t) ss.st_size, "read clone");
	expect(strcmp(buf, "reflink payload\n") == 0, "clone contents");
	close(fd);
}

int main(void)
{
	char src[] = "/tmp/clonefile-reflink-src-XXXXXX";
	char dst[] = "/tmp/clonefile-reflink-dst-XXXXXX";
	char dst2[] = "/tmp/clonefile-reflink-fd-XXXXXX";
	char link[] = "/tmp/clonefile-reflink-link-XXXXXX";
	char linkDst[] = "/tmp/clonefile-reflink-linkdst-XXXXXX";
	int fd, srcFd;

	fd = mkstemp(src);
	expect(fd >= 0, "scratch source");
	expect(write(fd, "reflink payload\n", 16) == 16, "write source");
	close(fd);
	expect(chmod(src, 0640) == 0, "source mode");

	if (clonefileat(AT_FDCWD, src, AT_FDCWD, dst, 0) != 0)
	{
		if (errno == EOPNOTSUPP) {
			unlink(src);
			printf("SKIP: host filesystem does not support reflink\n");
			return 0;
		}
		printf("FAIL: clonefileat (errno %d)\n", errno);
		return 1;
	}
	expectUnchanged(src, dst);

	srcFd = open(src, O_RDONLY);
	expect(srcFd >= 0, "open source descriptor");
	expect(fclonefileat(srcFd, AT_FDCWD, dst2, 0) == 0, "fclonefileat");
	close(srcFd);
	expectUnchanged(src, dst2);

	errno = 0;
	expect(clonefileat(AT_FDCWD, src, AT_FDCWD, dst, 0) == -1 && errno == EEXIST,
		"a clone does not replace an existing destination");
	errno = 0;
	expect(clonefileat(AT_FDCWD, "/tmp/no-such-clonefile-source", AT_FDCWD, linkDst, 0) == -1 &&
		errno == ENOENT, "a missing source is ENOENT");

	close(mkstemp(link));
	expect(unlink(link) == 0, "remove scratch link target");
	expect(symlink(src, link) == 0, "source symbolic link");
	errno = 0;
	expect(clonefileat(AT_FDCWD, link, AT_FDCWD, linkDst, CLONE_NOFOLLOW) == -1,
		"CLONE_NOFOLLOW does not clone a symbolic link");
	expect(lstat(linkDst, &(struct stat){ 0 }) != 0, "no destination for a refused link clone");

	unlink(link);
	unlink(src);
	unlink(dst);
	unlink(dst2);
	printf("PASS: clonefileat and fclonefileat use a host reflink\n");
	return 0;
}
