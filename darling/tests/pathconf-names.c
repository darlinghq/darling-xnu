// Copyright (C) 2026 Lubos Dolezel
// pathconf() and fpathconf() answer the Darwin _PC_ names 10 to 27 (sys/unistd.h) as macOS does for a path on an
// APFS volume (values observed on macOS 26.7.3 and 10.14.4), and still reject names the system does not define.
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

static int failures;

static void check(const char* label, const char* path, int fd, int name, long want)
{
	errno = 0;
	long byPath = pathconf(path, name);
	int pathErrno = errno;
	errno = 0;
	long byFd = fpathconf(fd, name);
	int fdErrno = errno;
	if (want == -1) {
		if (byPath != -1 || pathErrno != EINVAL || byFd != -1 || fdErrno != EINVAL) {
			printf("FAIL %s (%d): want EINVAL, got pathconf %ld errno %d, fpathconf %ld errno %d\n", label, name, byPath, pathErrno, byFd, fdErrno);
			failures++;
		}
	} else if (byPath != want || byFd != want) {
		printf("FAIL %s (%d): want %ld, got pathconf %ld, fpathconf %ld\n", label, name, want, byPath, byFd);
		failures++;
	}
}

int main(void)
{
	const char* path = "/tmp";
	int fd = open(path, O_RDONLY);
	if (fd < 0) {
		printf("FAIL: cannot open %s\n", path);
		return 1;
	}
	check("_PC_NAME_CHARS_MAX", path, fd, _PC_NAME_CHARS_MAX, 255);
	check("_PC_CASE_SENSITIVE", path, fd, _PC_CASE_SENSITIVE, 0);
	check("_PC_CASE_PRESERVING", path, fd, _PC_CASE_PRESERVING, 1);
	check("_PC_EXTENDED_SECURITY_NP", path, fd, _PC_EXTENDED_SECURITY_NP, 0);
	check("_PC_AUTH_OPAQUE_NP", path, fd, _PC_AUTH_OPAQUE_NP, 0);
	check("_PC_2_SYMLINKS", path, fd, _PC_2_SYMLINKS, 1);
	check("_PC_ALLOC_SIZE_MIN", path, fd, _PC_ALLOC_SIZE_MIN, 1);
	check("_PC_ASYNC_IO", path, fd, _PC_ASYNC_IO, 1);
	check("_PC_FILESIZEBITS", path, fd, _PC_FILESIZEBITS, 64);
	check("_PC_PRIO_IO", path, fd, _PC_PRIO_IO, 0);
	check("_PC_REC_INCR_XFER_SIZE", path, fd, _PC_REC_INCR_XFER_SIZE, 4096);
	check("_PC_REC_MAX_XFER_SIZE", path, fd, _PC_REC_MAX_XFER_SIZE, 65536);
	check("_PC_REC_MIN_XFER_SIZE", path, fd, _PC_REC_MIN_XFER_SIZE, 4096);
	check("_PC_REC_XFER_ALIGN", path, fd, _PC_REC_XFER_ALIGN, 4096);
	check("_PC_SYMLINK_MAX", path, fd, _PC_SYMLINK_MAX, 255);
	check("_PC_SYNC_IO", path, fd, _PC_SYNC_IO, 0);
	check("_PC_XATTR_SIZE_BITS", path, fd, _PC_XATTR_SIZE_BITS, 64);
	check("_PC_MIN_HOLE_SIZE", path, fd, _PC_MIN_HOLE_SIZE, 4096);
	check("name 0", path, fd, 0, -1);
	check("name 28", path, fd, 28, -1);
	check("name 1000", path, fd, 1000, -1);
	check("negative name", path, fd, -1, -1);
	close(fd);
	if (failures)
		return 1;
	puts("PASS: pathconf answers the Darwin _PC_ names 10 to 27");
	return 0;
}
