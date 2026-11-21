// Copyright (C) 2026 Lubos Dolezel
// pathconf() and fpathconf() answer every _PC_ name (sys/unistd.h) the way macOS does for objects on an APFS volume,
// with a table per kind of object (regular file and directory, FIFO, character device, anonymous pipe and socket),
// and report the errors the documented contract gives: ENOENT/ENOTDIR for a bad path, EBADF for a bad descriptor,
// EINVAL for a name the system does not define (or does not answer for that kind of object).
// Values observed on macOS 26.7.3 and 10.14.4 (black-box); _PC_EXTENDED_SECURITY_NP is 0 on purpose (the guest has
// no access control lists; macOS says 1).
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#define X (-1) /* EINVAL */

static int failures;

// Index 0 is unused; entries 1 to 27 are the names of sys/unistd.h in order.
static const long file_values[28] = {
	X, 32767, X, X, 255, 1024, 512, 200112, 200112, X,
	255, 0, 1, 0, 0, 1, 1, 1, 64, 0,
	4096, 65536, 4096, 4096, 255, 0, 64, 4096
};
static const long fifo_values[28] = {
	X, 32767, X, X, X, X, 512, 200112, X, X,
	X, X, X, 0, 0, 1, 1, 1, X, 0,
	4096, 65536, 4096, 4096, 255, 0, X, X
};
static const long device_values[28] = {
	X, 32767, 1024, 1024, X, X, 512, 200112, X, 255,
	X, X, X, 0, 0, 1, 1, 1, X, 0,
	4096, 65536, 4096, 4096, 255, 0, 31, X
};
static const long anonymous_values[28] = {
	X, X, X, X, X, X, 512, X, X, X,
	X, X, X, X, X, X, X, X, X, X,
	X, X, X, X, X, X, X, X
};

static void expect(const char* what, int name, long got, int err, long want, int wantErr)
{
	if (want == X) {
		if (got != -1 || err != wantErr) {
			printf("FAIL %s name %d: want errno %d, got %ld errno %d\n", what, name, wantErr, got, err);
			failures++;
		}
	} else if (got != want) {
		printf("FAIL %s name %d: want %ld, got %ld (errno %d)\n", what, name, want, got, err);
		failures++;
	}
}

// path may be NULL (descriptor only) and fd may be -1 (path only).
static void check_kind(const char* what, const char* path, int fd, const long* values)
{
	for (int name = 1; name <= 27; name++) {
		long r;
		if (path) {
			errno = 0;
			r = pathconf(path, name);
			expect(what, name, r, errno, values[name], EINVAL);
		}
		if (fd >= 0) {
			errno = 0;
			r = fpathconf(fd, name);
			expect(what, name, r, errno, values[name], EINVAL);
		}
	}
	// Names the system does not define, for a valid object.
	static const int bad[] = { 0, 28, 30, 1000, -1 };
	for (unsigned i = 0; i < sizeof(bad) / sizeof(bad[0]); i++) {
		if (path) {
			errno = 0;
			expect(what, bad[i], pathconf(path, bad[i]), errno, X, EINVAL);
		}
		if (fd >= 0) {
			errno = 0;
			expect(what, bad[i], fpathconf(fd, bad[i]), errno, X, EINVAL);
		}
	}
}

static void check_path_error(const char* path, int wantErr)
{
	static const int names[] = { 1, 4, 5, 6, 7, 8, 10, 27, 0, 99 };
	for (unsigned i = 0; i < sizeof(names) / sizeof(names[0]); i++) {
		errno = 0;
		expect(path, names[i], pathconf(path, names[i]), errno, X, wantErr);
	}
}

int main(void)
{
	char dir[] = "/tmp/pathconf-names.XXXXXX";
	char file[128], fifo[128], link[128], dangling[128], missing[128], underfile[160];
	if (!mkdtemp(dir)) {
		printf("FAIL: mkdtemp\n");
		return 1;
	}
	snprintf(file, sizeof(file), "%s/file", dir);
	snprintf(fifo, sizeof(fifo), "%s/fifo", dir);
	snprintf(link, sizeof(link), "%s/link", dir);
	snprintf(dangling, sizeof(dangling), "%s/dangling", dir);
	snprintf(missing, sizeof(missing), "%s/missing", dir);
	snprintf(underfile, sizeof(underfile), "%s/x", file);

	int filefd = open(file, O_CREAT | O_RDWR, 0600);
	int dirfd = open(dir, O_RDONLY);
	int fifofd = -1;
	int nulfd = open("/dev/null", O_RDWR);
	int pipefds[2] = { -1, -1 };
	int sockfds[2] = { -1, -1 };
	if (filefd < 0 || dirfd < 0 || nulfd < 0 || mkfifo(fifo, 0600) != 0 || (fifofd = open(fifo, O_RDWR)) < 0 ||
	    symlink("file", link) != 0 || symlink("nonexistent", dangling) != 0 || pipe(pipefds) != 0 ||
	    socketpair(AF_UNIX, SOCK_STREAM, 0, sockfds) != 0) {
		printf("FAIL: cannot set up the test objects\n");
		return 1;
	}

	check_kind("regular file", file, filefd, file_values);
	check_kind("directory", dir, dirfd, file_values);
	check_kind("symlink to a file", link, -1, file_values);
	check_kind("fifo", fifo, fifofd, fifo_values);
	check_kind("/dev/null", "/dev/null", nulfd, device_values);
	check_kind("pipe read end", NULL, pipefds[0], anonymous_values);
	check_kind("pipe write end", NULL, pipefds[1], anonymous_values);
	check_kind("socket", NULL, sockfds[0], anonymous_values);

	// A path that does not resolve is reported before the name is looked at.
	check_path_error(missing, ENOENT);
	check_path_error(dangling, ENOENT);
	check_path_error("", ENOENT);
	check_path_error(underfile, ENOTDIR);

	// A descriptor that is not open is EBADF whatever the name.
	for (int name = 0; name <= 28; name++) {
		errno = 0;
		expect("closed descriptor", name, fpathconf(9999, name), errno, X, EBADF);
	}
	close(filefd);
	errno = 0;
	expect("closed descriptor", 5, fpathconf(filefd, 5), errno, X, EBADF);
	errno = 0;
	expect("negative descriptor", 5, fpathconf(-1, 5), errno, X, EBADF);

	close(dirfd);
	close(fifofd);
	close(nulfd);
	close(pipefds[0]);
	close(pipefds[1]);
	close(sockfds[0]);
	close(sockfds[1]);
	unlink(file);
	unlink(fifo);
	unlink(link);
	unlink(dangling);
	rmdir(dir);

	if (failures)
		return 1;
	puts("PASS: pathconf and fpathconf answer the Darwin _PC_ names per object kind and report bad paths and descriptors");
	return 0;
}
