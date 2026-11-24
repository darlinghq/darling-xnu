// O_CREAT|O_EXCL never follows a final symbolic link (POSIX open(2)): onto a dangling or a live link it fails with
// EEXIST and creates or truncates nothing. Without O_EXCL the link is followed and the target created. O_NOFOLLOW on a
// link fails with ELOOP. Creating a fifo, directory, node, hard link or symbolic link onto the name of a dangling link
// fails with EEXIST. Following the link when O_EXCL is set lets a planted link in a shared directory redirect the create.
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static int failures;

static void expect_errno(const char* what, int result, int err)
{
	int ok = result == -1 && errno == err;
	if (!ok)
		printf("FAIL: %s: result %d, errno %d (want -1, %d)\n", what, result, result == -1 ? errno : 0, err);
	else
		printf("PASS: %s\n", what);
	failures += !ok;
}

static void expect_true(const char* what, int ok)
{
	printf("%s: %s\n", ok ? "PASS" : "FAIL", what);
	failures += !ok;
}

static int exists(const char* path)
{
	struct stat st;
	return lstat(path, &st) == 0;
}

static char dir[64];

static char* p(const char* name)
{
	static char buf[8][128];
	static int i;
	i = (i + 1) % 8;
	snprintf(buf[i], sizeof buf[i], "%s/%s", dir, name);
	return buf[i];
}

// a fresh dangling link "link" -> "target"; target does not exist
static void fresh(void)
{
	unlink(p("link"));
	unlink(p("target"));
	unlink(p("other"));
	rmdir(p("other"));
	symlink("target", p("link"));
}

int main(void)
{
	strcpy(dir, "/tmp/open-excl-symlink-XXXXXX");
	if (!mkdtemp(dir)) {
		printf("FAIL: scratch directory\n");
		return 1;
	}
	int fd, dfd = open(dir, O_RDONLY | O_DIRECTORY);

	fresh();
	errno = 0;
	fd = open(p("link"), O_CREAT | O_EXCL | O_WRONLY, 0644);
	expect_errno("open O_CREAT|O_EXCL on a dangling link", fd, EEXIST);
	if (fd >= 0) close(fd);
	expect_true("  the link's target was not created", !exists(p("target")));

	fresh();
	errno = 0;
	fd = openat(dfd, "link", O_CREAT | O_EXCL | O_WRONLY, 0644);
	expect_errno("openat O_CREAT|O_EXCL on a dangling link", fd, EEXIST);
	if (fd >= 0) close(fd);
	expect_true("  the link's target was not created", !exists(p("target")));

	fresh();
	errno = 0;
	fd = open(p("link"), O_CREAT | O_EXCL | O_NOFOLLOW | O_WRONLY, 0644);
	expect_errno("open O_CREAT|O_EXCL|O_NOFOLLOW on a dangling link", fd, EEXIST);
	if (fd >= 0) close(fd);
	expect_true("  the link's target was not created", !exists(p("target")));

	// a link to an existing file: the file is not opened or truncated
	fresh();
	fd = open(p("target"), O_CREAT | O_WRONLY, 0644);
	write(fd, "keep", 4);
	close(fd);
	errno = 0;
	fd = open(p("link"), O_CREAT | O_EXCL | O_TRUNC | O_WRONLY, 0644);
	expect_errno("open O_CREAT|O_EXCL|O_TRUNC on a link to a file", fd, EEXIST);
	if (fd >= 0) close(fd);
	struct stat st;
	expect_true("  the file was not truncated", stat(p("target"), &st) == 0 && st.st_size == 4);

	// O_NOFOLLOW alone on a link
	errno = 0;
	fd = open(p("link"), O_RDONLY | O_NOFOLLOW);
	expect_errno("open O_NOFOLLOW on a link", fd, ELOOP);
	if (fd >= 0) close(fd);
	errno = 0;
	fd = openat(dfd, "link", O_RDONLY | O_NOFOLLOW);
	expect_errno("openat O_NOFOLLOW on a link", fd, ELOOP);
	if (fd >= 0) close(fd);

	// without O_EXCL the link is followed and the target created
	fresh();
	fd = open(p("link"), O_CREAT | O_WRONLY, 0644);
	expect_true("open O_CREAT on a dangling link follows it", fd >= 0 && exists(p("target")));
	if (fd >= 0) close(fd);

	// O_EXCL on a plain name still creates, and on an existing file fails
	fresh();
	fd = open(p("other"), O_CREAT | O_EXCL | O_WRONLY, 0644);
	expect_true("open O_CREAT|O_EXCL on a free name creates it", fd >= 0);
	if (fd >= 0) close(fd);
	errno = 0;
	fd = open(p("other"), O_CREAT | O_EXCL | O_WRONLY, 0644);
	expect_errno("open O_CREAT|O_EXCL on an existing file", fd, EEXIST);
	if (fd >= 0) close(fd);

	// other creating calls onto the name of a dangling link
	fresh();
	errno = 0;
	expect_errno("mkfifo onto a dangling link", mkfifo(p("link"), 0600), EEXIST);
	expect_true("  the link's target was not created", !exists(p("target")));
	errno = 0;
	expect_errno("mkdir onto a dangling link", mkdir(p("link"), 0700), EEXIST);
	expect_true("  the link's target was not created", !exists(p("target")));
	errno = 0;
	expect_errno("mknod (fifo) onto a dangling link", mknod(p("link"), S_IFIFO | 0600, 0), EEXIST);
	expect_true("  the link's target was not created", !exists(p("target")));
	errno = 0;
	expect_errno("symlink onto a dangling link", symlink("x", p("link")), EEXIST);
	errno = 0;
	expect_errno("symlinkat onto a dangling link", symlinkat("x", dfd, "link"), EEXIST);

	fd = open(p("src"), O_CREAT | O_WRONLY, 0644);
	if (fd >= 0) close(fd);
	errno = 0;
	expect_errno("link onto a dangling link", link(p("src"), p("link")), EEXIST);
	expect_true("  the link's target was not created", !exists(p("target")));
	errno = 0;
	expect_errno("linkat onto a dangling link", linkat(dfd, "src", dfd, "link", 0), EEXIST);
	expect_true("  the link's target was not created", !exists(p("target")));

	unlink(p("link")); unlink(p("target")); unlink(p("other")); unlink(p("src"));
	close(dfd);
	rmdir(dir);
	printf(failures ? "FAIL: %d checks failed\n" : "PASS: all checks\n", failures);
	return failures != 0;
}
