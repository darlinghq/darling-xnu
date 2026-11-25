// statfs-fsid <path>...
//
// f_fsid identifies the volume a path is on. On macOS its first value is the volume's device number, the st_dev that
// stat() reports for files on it, so two different volumes never share it. Observed on macOS 26.7.3 and 10.14.4 (both
// x86_64) for paths on APFS, HFS+ and devfs. The root directory itself on macOS 10.15+ is the exception (a firmlink
// gives it the data volume's st_dev), so the runs there leave it out.
//
// For each path: statfs().f_fsid.val[0] equals stat().st_dev, fstatfs() gives the same f_fsid as statfs(), and the
// getfsstat() entry of the same mount point, if the list has one, has that f_fsid. For each pair of paths whose st_dev
// differ, the f_fsid differ too. Prints PASS/FAIL lines; exits 0 when everything passed.
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <unistd.h>

static int failures;

static void check(int ok, const char* what, const char* path)
{
	printf("%s: %s (%s)\n", ok ? "PASS" : "FAIL", what, path);
	if (!ok)
		failures++;
}

int main(int argc, char** argv)
{
	int count = argc - 1;
	struct statfs* volumes = calloc(count, sizeof(*volumes));
	struct stat* files = calloc(count, sizeof(*files));
	int listed = getfsstat(NULL, 0, MNT_NOWAIT);
	struct statfs* list = calloc(listed > 0 ? listed : 1, sizeof(*list));

	listed = getfsstat(list, listed * (int) sizeof(*list), MNT_NOWAIT);
	if (listed <= 0)
		check(0, "getfsstat lists the mounts", "");

	for (int i = 0; i < count; i++)
	{
		const char* path = argv[i + 1];
		struct statfs viaFd;
		int fd;
		char what[200];

		if (statfs(path, &volumes[i]) != 0 || stat(path, &files[i]) != 0)
		{
			perror(path);
			failures++;
			continue;
		}
		snprintf(what, sizeof(what), "f_fsid.val[0] %#x is st_dev %#x", volumes[i].f_fsid.val[0], files[i].st_dev);
		check(volumes[i].f_fsid.val[0] == (int32_t) files[i].st_dev, what, path);

		fd = open(path, O_RDONLY);
		check(fd >= 0 && fstatfs(fd, &viaFd) == 0
			&& memcmp(&viaFd.f_fsid, &volumes[i].f_fsid, sizeof(fsid_t)) == 0, "fstatfs gives the same f_fsid", path);
		if (fd >= 0)
			close(fd);

		int found = 0;
		for (int j = 0; j < listed; j++)
		{
			if (strcmp(list[j].f_mntonname, volumes[i].f_mntonname) == 0)
				found = memcmp(&list[j].f_fsid, &volumes[i].f_fsid, sizeof(fsid_t)) == 0 ? 1 : -1;
		}
		check(found != -1, "the mount list has no other f_fsid for the mount point", path);
	}

	for (int i = 0; i < count; i++)
	{
		for (int j = i + 1; j < count; j++)
		{
			char pair[2100];

			if (files[i].st_dev == files[j].st_dev)
				continue;
			snprintf(pair, sizeof(pair), "%s and %s", argv[i + 1], argv[j + 1]);
			check(memcmp(&volumes[i].f_fsid, &volumes[j].f_fsid, sizeof(fsid_t)) != 0,
				"different devices have different f_fsid", pair);
		}
	}

	free(list);
	free(files);
	free(volumes);
	return failures != 0;
}
