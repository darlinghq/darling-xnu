#include <darling/emulation/xnu_syscall/bsd/impl/stat/mount_table.h>

#include <sys/errno.h>
#include <sys/mman.h>
#include <libsimple/lock.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/common/guarded/table.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/conversion/duct_errno.h>
#include <darling/emulation/conversion/fcntl/open.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/xnu_syscall/bsd/impl/fcntl/open.h>
#include <darling/emulation/xnu_syscall/bsd/impl/mman/mman.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/close.h>

extern char *strtok_r(char *str, const char *delim, char **saveptr);
extern __SIZE_TYPE__ strlcpy(char* dst, const char* src, __SIZE_TYPE__ size);
extern __SIZE_TYPE__ strlen(const char* str);
extern int strncmp(const char* s1, const char* s2, __SIZE_TYPE__ len);
extern void* memcpy(void* dst, const void* src, __SIZE_TYPE__ n);
extern void* memchr(const void* s, int c, __SIZE_TYPE__ n);

#define LINUX_POLLPRI 0x002
#define LINUX_POLLERR 0x008
#define LINUX_POLLNVAL 0x020

struct linux_pollfd {
	int fd;
	short events;
	short revents;
};

//
// statfs() and fstatfs() look up the mount containing a path in /proc/self/mounts.
// The table rarely changes, so its text is kept between calls, and so is the
// descriptor: Linux reports POLLPRI (and POLLERR) on an open /proc/self/mounts
// after a mount or unmount (see proc(5)), so the text is read again exactly when
// the table has changed.
//
// The descriptor is guarded against being closed by the program and is closed in
// a forked child (whose open file description would otherwise be shared with the
// parent, and with it the change notification). In dyld, or if the descriptor
// can't be guarded, the table is read for every lookup as before.
//

static libsimple_lock_t mount_table_lock = LIBSIMPLE_LOCK_INITIALIZER;
static int mount_table_fd = -1;
static int mount_table_valid = 0;
static char* mount_table_text = NULL;
static size_t mount_table_length = 0;
static size_t mount_table_capacity = 0;

#ifndef VARIANT_DYLD
// called for the guarded descriptor in a forked child
static void mount_table_forget(int fd)
{
	close_internal(fd);
	mount_table_fd = -1;
	mount_table_valid = 0;
	libsimple_lock_init(&mount_table_lock);
}
#endif

static long mount_table_grow_locked(void)
{
	size_t capacity = mount_table_capacity ? mount_table_capacity * 2 : 16384;
	// mapped rather than allocated: libsystem_kernel sits below malloc
	char* text = sys_mmap(NULL, capacity, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);

	if ((unsigned long) text > -4096ul)
		return -ENOMEM;

	if (mount_table_text != NULL)
	{
		memcpy(text, mount_table_text, mount_table_length);
		sys_munmap(mount_table_text, mount_table_capacity);
	}

	mount_table_text = text;
	mount_table_capacity = capacity;
	return 0;
}

static long mount_table_read_locked(int fd)
{
	long ret;

	ret = LINUX_SYSCALL(__NR_lseek, fd, 0, 0 /* SEEK_SET */);
	if (ret < 0)
		return errno_linux_to_bsd(ret);

	mount_table_length = 0;
	for (;;)
	{
		if (mount_table_length == mount_table_capacity)
		{
			ret = mount_table_grow_locked();
			if (ret < 0)
				return ret;
		}

		ret = LINUX_SYSCALL(__NR_read, fd, mount_table_text + mount_table_length, mount_table_capacity - mount_table_length);
		if (ret == -LINUX_EINTR)
			continue;
		if (ret < 0)
			return errno_linux_to_bsd(ret);
		if (ret == 0)
			break;

		mount_table_length += ret;
	}

	mount_table_valid = 1;
	return 0;
}

// makes sure the cached text is current; returns 0 or a negative BSD errno
static long mount_table_update_locked(void)
{
	if (mount_table_fd >= 0)
	{
		struct linux_pollfd pfd = { .fd = mount_table_fd, .events = LINUX_POLLPRI, .revents = 0 };
		long ret = LINUX_SYSCALL(__NR_poll, &pfd, 1, 0);

		if (ret > 0 && (pfd.revents & LINUX_POLLNVAL))
		{
			// the descriptor is gone after all (e.g. replaced with dup2); start over
			guard_table_remove(mount_table_fd);
			mount_table_fd = -1;
			mount_table_valid = 0;
		}
		else if (ret > 0 && (pfd.revents & (LINUX_POLLPRI | LINUX_POLLERR)))
		{
			mount_table_valid = 0;
		}
	}

	if (mount_table_fd < 0)
	{
		int fd = sys_open_nocancel("/proc/self/mounts", BSD_O_RDONLY | BSD_O_CLOEXEC, 0);
		if (fd < 0)
			return fd;

#ifdef VARIANT_DYLD
		// dyld's copy of this code has its own guard table, which the program's
		// close() does not consult, so don't leave a descriptor behind
		if (1)
#else
		guard_entry_options_t options = { .close = mount_table_forget };
		if (guard_table_add(fd, guard_flag_prevent_close | guard_flag_close_on_fork, &options) != 0)
#endif
		{
			// can't keep it open; read the table just for this lookup
			long ret = mount_table_read_locked(fd);
			close_internal(fd);
			mount_table_valid = 0;
			return ret < 0 ? ret : 1;
		}

		mount_table_fd = fd;
		mount_table_valid = 0;
	}

	if (!mount_table_valid)
		return mount_table_read_locked(mount_table_fd);

	return 0;
}

long mount_table_lookup(const char* path, char* mnton, size_t mnton_size,
	char* fstype, size_t fstype_size, char* mntfrom, size_t mntfrom_size)
{
	long ret;
	size_t offset = 0;
	int max_len = 0;

	mnton[0] = 0;
	fstype[0] = 0;
	mntfrom[0] = 0;

	libsimple_lock_lock(&mount_table_lock);

	ret = mount_table_update_locked();
	if (ret < 0)
		goto out;

	// the mount point is the longest one that is a prefix of the path; of equal ones the last (the topmost)
	while (offset < mount_table_length)
	{
		char line[1024 + 512];
		const char* start = mount_table_text + offset;
		const char* newline = memchr(start, '\n', mount_table_length - offset);
		size_t length = newline ? (size_t) (newline - start) : mount_table_length - offset;
		char* p;
		char* saveptr;
		char* from;
		int len;

		offset += length + 1;

		if (length >= sizeof(line))
			length = sizeof(line) - 1;
		memcpy(line, start, length);
		line[length] = 0;

		p = strtok_r(line, " ", &saveptr);
		if (p == NULL)
			continue;

		from = p;
		p = strtok_r(NULL, " ", &saveptr);
		if (p == NULL)
			continue;

		len = strlen(p);
		if (strncmp(p, path, len) != 0 || len < max_len)
			continue;

		max_len = len;
		strlcpy(mnton, p, mnton_size);

		p = strtok_r(NULL, " ", &saveptr);
		if (p == NULL)
			continue;

		strlcpy(fstype, p, fstype_size);
		strlcpy(mntfrom, from, mntfrom_size);
	}

	ret = 0;

out:
	libsimple_lock_unlock(&mount_table_lock);
	return ret;
}
