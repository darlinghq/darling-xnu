// Copyright (C) 2026 Lubos Dolezel
// statfs, fstatfs and getfsstat describe the volumes a guest can use: the volume holding "/" is named "/" by
// statfs, fstatfs and getfsstat alike (the root volume is the first getfsstat entry), getfsstat lists only mounts
// whose names are usable in the guest (not the host's other mounts, which statfs names below /Volumes/SystemRoot),
// the count with and without a buffer agree, and fields the system has no value for are zero rather than whatever
// the caller's buffer held. The program also passes on macOS.
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <unistd.h>

static int failures;
#define CHECK(c) do { if (!(c)) { printf("FAIL: %s (line %d, errno %d)\n", #c, __LINE__, errno); failures++; } } while (0)

#define HOST_ALIAS "/Volumes/SystemRoot"

int main(void)
{
	struct statfs info;

	// fields without a value are cleared
	memset(&info, 0xAA, sizeof info);
	CHECK(statfs("/", &info) == 0);
	CHECK(strcmp(info.f_mntonname, "/") == 0);
	CHECK(info.f_owner == 0);
	CHECK(info.f_fstypename[0] != 0);

	// fstatfs on "/" gives the same names as statfs
	{
		struct statfs viaFd;
		int fd = open("/", O_RDONLY);
		CHECK(fd >= 0);
		memset(&viaFd, 0xAA, sizeof viaFd);
		CHECK(fstatfs(fd, &viaFd) == 0);
		CHECK(strcmp(viaFd.f_mntonname, info.f_mntonname) == 0);
		CHECK(strcmp(viaFd.f_fstypename, info.f_fstypename) == 0);
		CHECK(strcmp(viaFd.f_mntfromname, info.f_mntfromname) == 0);
		CHECK(viaFd.f_owner == 0);
		close(fd);
	}

	// a path below "/" is reported on the volume that holds it, named as getfsstat names it
	{
		struct statfs below;
		CHECK(statfs("/usr", &below) == 0);
		CHECK(below.f_mntonname[0] == '/');
		CHECK(strncmp(below.f_mntonname, HOST_ALIAS, strlen(HOST_ALIAS)) != 0);
	}

	// missing paths and bad descriptors keep their errors
	errno = 0;
	CHECK(statfs("/nonexistent-statfs-test-path", &info) == -1 && errno == ENOENT);
	errno = 0;
	CHECK(fstatfs(-1, &info) == -1 && errno == EBADF);

	// getfsstat
	int count = getfsstat(NULL, 0, MNT_NOWAIT);
	CHECK(count >= 1);
	struct statfs* list = calloc(count + 2, sizeof *list);
	int filled = getfsstat(list, (count + 2) * sizeof *list, MNT_NOWAIT);
	CHECK(filled == count);
	CHECK(filled >= 1 && strcmp(list[0].f_mntonname, "/") == 0);
	for (int i = 0; i < filled; i++) {
		CHECK(list[i].f_mntonname[0] == '/');
		CHECK(strncmp(list[i].f_mntonname, HOST_ALIAS, strlen(HOST_ALIAS)) != 0);
		for (int j = i + 1; j < filled; j++)
			CHECK(strcmp(list[i].f_mntonname, list[j].f_mntonname) != 0);
	}
	// a buffer that holds one entry gets one, a buffer too small for one gets none
	CHECK(getfsstat(list, sizeof *list, MNT_NOWAIT) == 1);
	CHECK(getfsstat(list, sizeof *list - 1, MNT_NOWAIT) == 0);
	free(list);

	// a host mount outside the guest tree is named below /Volumes/SystemRoot by statfs (Darling only)
	if (access(HOST_ALIAS "/proc", F_OK) == 0) {
		struct statfs host;
		CHECK(statfs(HOST_ALIAS "/proc", &host) == 0);
		CHECK(strcmp(host.f_mntonname, "/") == 0 || strncmp(host.f_mntonname, HOST_ALIAS, strlen(HOST_ALIAS)) == 0);
	}

	if (failures)
		return 1;
	puts("PASS: statfs, fstatfs and getfsstat describe the guest's volumes");
	return 0;
}
