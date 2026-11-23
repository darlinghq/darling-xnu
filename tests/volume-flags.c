// Copyright (C) 2026 Lubos Dolezel
// volume-flags <path>...: prints "<path>: <f_mntonname> <hex flags> <names>" for each path, from statfs(2). The
// names are the set flags among LOCAL ROOTFS DONTBROWSE REMOVABLE RDONLY, in that order. Also builds on macOS.
#include <stdio.h>
#include <sys/mount.h>

int main(int argc, char** argv)
{
	static const struct { unsigned int flag; const char* name; } names[] = {
		{ MNT_LOCAL, "LOCAL" },
		{ MNT_ROOTFS, "ROOTFS" },
		{ MNT_DONTBROWSE, "DONTBROWSE" },
		{ MNT_REMOVABLE, "REMOVABLE" },
		{ MNT_RDONLY, "RDONLY" },
	};
	int result = 0;

	for (int i = 1; i < argc; i++)
	{
		struct statfs buf;

		if (statfs(argv[i], &buf) != 0)
		{
			perror(argv[i]);
			result = 1;
			continue;
		}
		printf("%s: %s %x", argv[i], buf.f_mntonname, (unsigned int) buf.f_flags);
		for (size_t n = 0; n < sizeof(names) / sizeof(names[0]); n++)
		{
			if (buf.f_flags & names[n].flag)
				printf(" %s", names[n].name);
		}
		printf("\n");
	}
	return result;
}
