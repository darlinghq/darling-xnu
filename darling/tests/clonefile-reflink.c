// clonefileat()/fclonefileat() contract. A clone made on a reflink-capable filesystem must be a
// distinct file with the source's contents, mode and times; ENOSYS and a plain-data copy are not
// allowed. A filesystem that cannot reflink reports ENOTSUP, and then that part is skipped (the other
// checks do not need a reflink and always run).
// The checks follow what clonefile(2) does on macOS 26.7.3 and 10.14.4 (black-box observation).
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/clonefile.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

static int g_failures = 0;
static char g_dir[] = "/tmp/clonefile-reflink-XXXXXX";

#define CHECK(cond) do { if (!(cond)) { printf("FAIL %d: %s (errno %d)\n", __LINE__, #cond, errno); g_failures++; } } while (0)

static const char* path(const char* name)
{
	static char buffers[6][256];
	static int next;
	char* buffer = buffers[next++ % 6];

	snprintf(buffer, 256, "%s/%s", g_dir, name);
	return buffer;
}

static void write_file(const char* name, const char* contents, mode_t mode)
{
	int fd = open(path(name), O_CREAT | O_WRONLY | O_TRUNC, 0600);

	CHECK(fd >= 0);
	if (fd >= 0)
	{
		CHECK(write(fd, contents, strlen(contents)) == (ssize_t) strlen(contents));
		close(fd);
	}
	CHECK(chmod(path(name), mode) == 0);
}

static void check_clone_matches(const char* srcName, const char* dstName)
{
	struct stat ss, ds;
	char buf[64] = { 0 };
	int fd;

	CHECK(stat(path(srcName), &ss) == 0 && stat(path(dstName), &ds) == 0);
	CHECK(ss.st_ino != ds.st_ino);
	CHECK(ss.st_size == ds.st_size);
	CHECK((ss.st_mode & 07777) == (ds.st_mode & 07777));
	CHECK(ss.st_mtime == ds.st_mtime);
	CHECK(ss.st_atime == ds.st_atime);
	fd = open(path(dstName), O_RDONLY);
	CHECK(fd >= 0);
	if (fd >= 0)
	{
		CHECK(read(fd, buf, sizeof(buf) - 1) == (ssize_t) ss.st_size);
		CHECK(strcmp(buf, "reflink payload\n") == 0);
		close(fd);
	}
}

static int try_clone(const char* src, const char* dst, uint32_t flags)
{
	errno = 0;
	return clonefileat(AT_FDCWD, path(src), AT_FDCWD, path(dst), flags);
}

static void check_reflink(void)
{
	struct timeval times[2] = { { 1000000000, 0 }, { 1100000000, 0 } };
	int srcFd, dirFd;

	write_file("src", "reflink payload\n", 0640);
	CHECK(utimes(path("src"), times) == 0);

	if (try_clone("src", "clone", 0) != 0)
	{
		CHECK(errno == ENOTSUP);
		// A failed clone leaves nothing behind.
		CHECK(lstat(path("clone"), &(struct stat){ 0 }) != 0);
		printf("SKIP: host filesystem does not support reflink\n");
		return;
	}
	check_clone_matches("src", "clone");

	srcFd = open(path("src"), O_RDONLY);
	dirFd = open(g_dir, O_RDONLY);
	CHECK(srcFd >= 0 && dirFd >= 0);
	CHECK(fclonefileat(srcFd, dirFd, "fclone", 0) == 0);
	check_clone_matches("src", "fclone");
	CHECK(clonefileat(dirFd, "src", dirFd, "relative", 0) == 0);
	check_clone_matches("src", "relative");
	close(srcFd);
	close(dirFd);

	// A read-only source stays read-only.
	CHECK(chmod(path("src"), 0444) == 0);
	CHECK(try_clone("src", "readonly", 0) == 0);
	CHECK(chmod(path("src"), 0640) == 0);
	{
		struct stat st;
		CHECK(stat(path("readonly"), &st) == 0 && (st.st_mode & 07777) == 0444);
	}
	CHECK(try_clone("src", "noowner", CLONE_NOOWNERCOPY) == 0);
}

// These results do not depend on the filesystem being able to reflink.
static void check_errors(void)
{
	int srcFd, dirFd;

	write_file("plain", "reflink payload\n", 0640);

	// A destination that exists is never replaced. (macOS answers ENOTSUP first on a filesystem that cannot
	// clone, and EEXIST on APFS; this implementation looks at the destination first.)
	write_file("existing", "keep me\n", 0600);
	CHECK(try_clone("plain", "existing", 0) == -1 && (errno == EEXIST || errno == ENOTSUP));
	{
		char buf[16] = { 0 };
		int fd = open(path("existing"), O_RDONLY);
		CHECK(fd >= 0 && read(fd, buf, sizeof(buf) - 1) == 8 && strcmp(buf, "keep me\n") == 0);
		if (fd >= 0) close(fd);
	}

	CHECK(try_clone("missing", "x1", 0) == -1 && errno == ENOENT);
	CHECK(clonefileat(AT_FDCWD, "", AT_FDCWD, path("x2"), 0) == -1 && errno == ENOENT);
	CHECK(clonefileat(AT_FDCWD, path("plain"), AT_FDCWD, "", 0) == -1 && errno == ENOENT);
	CHECK(try_clone("plain", "no-such-directory/x3", 0) == -1 && errno == ENOENT);
	CHECK(try_clone("plain", "plain/x4", 0) == -1 && errno == ENOTDIR);

	// Flags that are not defined are EINVAL and nothing is created.
	CHECK(try_clone("plain", "x5", 0x80000000) == -1 && errno == EINVAL);
	CHECK(lstat(path("x5"), &(struct stat){ 0 }) != 0);

	// A file that is not a regular file or a directory cannot be cloned; opening a FIFO must not block.
	CHECK(mkfifo(path("fifo"), 0600) == 0);
	CHECK(try_clone("fifo", "x6", 0) == -1 && errno == EINVAL);
	CHECK(clonefileat(AT_FDCWD, "/dev/null", AT_FDCWD, path("x7"), 0) == -1 && errno == EINVAL);
	CHECK(lstat(path("x6"), &(struct stat){ 0 }) != 0 && lstat(path("x7"), &(struct stat){ 0 }) != 0);

	srcFd = open(path("plain"), O_RDONLY);
	dirFd = open(g_dir, O_RDONLY);
	CHECK(fclonefileat(srcFd, dirFd, "existing", 0) == -1 && (errno == EEXIST || errno == ENOTSUP));
	CHECK(fclonefileat(srcFd, dirFd, "", 0) == -1 && errno == ENOENT);
	CHECK(fclonefileat(9999, dirFd, "x8", 0) == -1 && errno == EBADF);
	close(srcFd);
	close(dirFd);
}

// Symbolic links: followed by default, cloned as a link with CLONE_NOFOLLOW.
static void check_symlinks(void)
{
	char target[256];
	ssize_t length;

	write_file("linktarget", "reflink payload\n", 0640);
	CHECK(symlink(path("linktarget"), path("link")) == 0);
	CHECK(symlink(path("nosuchtarget"), path("dangling")) == 0);

	CHECK(try_clone("link", "linkclone", CLONE_NOFOLLOW) == 0);
	{
		struct stat st;
		CHECK(lstat(path("linkclone"), &st) == 0 && S_ISLNK(st.st_mode));
	}
	length = readlink(path("linkclone"), target, sizeof(target) - 1);
	CHECK(length > 0);
	if (length > 0)
	{
		target[length] = '\0';
		CHECK(strcmp(target, path("linktarget")) == 0);
	}
	CHECK(try_clone("dangling", "danglingclone", CLONE_NOFOLLOW) == 0);
	{
		struct stat st;
		CHECK(lstat(path("danglingclone"), &st) == 0 && S_ISLNK(st.st_mode));
	}
	// Cloning a link again onto its clone is EEXIST.
	CHECK(try_clone("link", "linkclone", CLONE_NOFOLLOW) == -1 && errno == EEXIST);

	// Followed by default: a dangling link has no file to clone.
	CHECK(try_clone("dangling", "x10", 0) == -1 && errno == ENOENT);
	CHECK(lstat(path("x10"), &(struct stat){ 0 }) != 0);
}

int main(void)
{
	char command[300];

	if (mkdtemp(g_dir) == NULL)
	{
		perror("mkdtemp");
		return 1;
	}

	check_errors();
	check_symlinks();
	check_reflink();

	snprintf(command, sizeof(command), "rm -rf %s", g_dir);
	if (system(command) != 0)
		CHECK(!"cleanup");

	if (g_failures)
	{
		printf("%d check(s) failed\n", g_failures);
		return 1;
	}
	printf("PASS: clonefileat and fclonefileat\n");
	return 0;
}
