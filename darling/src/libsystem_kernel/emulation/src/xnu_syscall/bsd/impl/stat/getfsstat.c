#include <darling/emulation/xnu_syscall/bsd/impl/stat/getfsstat.h>

#include <stddef.h>

#include <darling/emulation/conversion/stat/common.h>
#include <darling/emulation/xnu_syscall/bsd/helper/stat/guest_mount.h>

long sys_getfsstat(struct bsd_statfs* buf, int bufsize, int flags)
{
	// the legacy structure; bufsize counts bytes of it
	return guest_mount_list_legacy(buf, bufsize);
}

// Whether some /proc/self/mounts mount point has st_dev dev.
static int getfsstat_dev_is_mount(unsigned long long dev)
{
	struct simple_readline_buf rbuf;
	char line[1024];
	int found = 0;
	int fd = sys_open("/proc/self/mounts", O_RDONLY, 0);
	if (fd < 0)
		return 0;
	__simple_readline_init(&rbuf);
	while (__simple_readline(fd, &rbuf, line, sizeof(line)))
	{
		char* saveptr;
		char* mnton = strtok_r(line, " ", &saveptr) ? strtok_r(NULL, " ", &saveptr) : NULL;
		struct linux_stat st;
		if (mnton == NULL)
			continue;
		statfs_unescape_mount_path(mnton);
		if (LINUX_SYSCALL(__NR_stat, mnton, &st) == 0 && st.st_dev == dev)
		{
			found = 1;
			break;
		}
	}
	close_internal(fd);
	return found;
}

long sys_getfsstat64(struct bsd_statfs64* buf, int bufsize, int flags)
{
	// the volumes the guest can use, named as the guest sees them, root volume first
	return guest_mount_list(buf, bufsize);
}
