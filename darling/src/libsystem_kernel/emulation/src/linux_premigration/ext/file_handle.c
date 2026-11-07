// This is needed for struct stat64 to exist alongside struct stat
// On ARM64, only 64-bit inodes are available, so struct stat is already the 64-bit-inode variant
#if !defined(__aarch64__) && !defined(__arm64__)
#define _DARWIN_NO_64_BIT_INODE
#define FILE_HANDLE_STAT_64 1
#endif

#include <darling/emulation/linux_premigration/ext/file_handle.h>

#include <sys/errno.h>
#include <sys/stat.h>

#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/conversion/fcntl/open.h>
#include <darling/emulation/common/base.h>
#include <darling/emulation/linux_premigration/vchroot_expand.h>
#include <darling/emulation/common/bsdthread/per_thread_wd.h>
#include <darling/emulation/xnu_syscall/bsd/impl/fcntl/open.h>
#include <darling/emulation/common/simple.h>
#include <darling/emulation/xnu_syscall/bsd/impl/stat/lstat.h>
#include <darling/emulation/xnu_syscall/bsd/impl/stat/stat.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/close.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

#include <darlingserver/rpc.h>

// A reference is the device and inode of a file, which needs no privileges to obtain and is the same
// however the file is reached, or by whichever process. Getting back to a path is another matter, as
// kernel file handles can only be opened with CAP_DAC_READ_SEARCH, which we don't have, and overlayfs
// may not provide them at all. So darlingserver remembers the last path used for each file.
#define MOUNT_ID_FILE_REF (-100)

#ifdef FILE_HANDLE_STAT_64
typedef struct stat64 file_handle_stat_t;
#define file_handle_stat sys_stat64
#define file_handle_lstat sys_lstat64
#else
typedef struct stat file_handle_stat_t;
#define file_handle_stat sys_stat
#define file_handle_lstat sys_lstat
#endif

static uint64_t ref_inode(const RefData* ref)
{
	return ((uint64_t) ref->inode_high << 32) | ref->inode_low;
}

VISIBLE
int sys_name_to_handle(const char* name, RefData* ref, int follow)
{
	int ret;

	struct vchroot_expand_args vc;
	vc.flags = follow ? VCHROOT_FOLLOW : 0;
	vc.dfd = get_perthread_wd();

	if (strlen(name) >= sizeof(vc.path))
		return -ENAMETOOLONG;

	strcpy(vc.path, name);
	ret = vchroot_expand(&vc);
	if (ret < 0)
		return errno_linux_to_bsd(ret);

	file_handle_stat_t st;
	ret = follow ? file_handle_stat(name, &st) : file_handle_lstat(name, &st);
	if (ret < 0)
		return ret;

	// Unused bytes must be the same too, for callers comparing references directly
	memset(ref, 0, sizeof(*ref));
	ref->mount_id = MOUNT_ID_FILE_REF;
	ref->device = st.st_dev;
	ref->inode_low = (uint32_t) st.st_ino;
	ref->inode_high = (uint32_t) (st.st_ino >> 32);

	ret = dserver_rpc_file_ref_register(st.st_dev, st.st_ino, vc.path, strlen(vc.path));
	if (ret < 0)
		return errno_linux_to_bsd(ret);

	return 0;
}

VISIBLE
int sys_handle_to_name(RefData* ref, char name[4096])
{
	// These are real kernel file handles, which we have no way to resolve
	if (ref->mount_id != MOUNT_ID_FILE_REF)
		return -ENOENT;

	struct vchroot_unexpand_args uargs;
	uint64_t length;

	int ret = dserver_rpc_file_ref_path(ref->device, ref_inode(ref), uargs.path, sizeof(uargs.path), &length);
	if (ret < 0)
		return errno_linux_to_bsd(ret);

	ret = vchroot_unexpand(&uargs);
	if (ret < 0)
		return errno_linux_to_bsd(ret);

	// The file may have been moved or deleted since, and something else may be there now
	file_handle_stat_t st;
	ret = file_handle_lstat(uargs.path, &st);
	if (ret < 0)
		return ret;

	if ((uint32_t) st.st_dev != ref->device || st.st_ino != ref_inode(ref))
		return -ENOENT;

	strlcpy(name, uargs.path, 4096);
	return 0;
}

VISIBLE
int sys_open_by_handle(RefData* ref, int flags)
{
	if (ref->mount_id == MOUNT_ID_FILE_REF)
	{
		char name[4096];

		int ret = sys_handle_to_name(ref, name);
		if (ret < 0)
			return ret;

		return sys_open_nocancel(name, flags, 0);
	}

	int linux_flags = oflags_bsd_to_linux(flags);

	if (sizeof(void*) == 4)
		linux_flags |= LINUX_O_LARGEFILE;

	// Now we need to find out the path of ref->mount_id
	struct simple_readline_buf rbuf;
	char line[1024];

	int fd_m = sys_open_nocancel("/proc/self/mountinfo", BSD_O_RDONLY, 0);
	if (fd_m < 0)
		return fd_m;
	const char* mount_path = NULL;
	
	__simple_readline_init(&rbuf);

	while (__simple_readline(fd_m, &rbuf, line, sizeof(line)))
	{
		char *p, *saveptr;

		p = strtok_r(line, " ", &saveptr);
		if (p == NULL)
			continue;

		if (__simple_atoi(p, NULL) != ref->mount_id)
			continue;

		for (int i = 0; i < 4; i++)
		{
			p = strtok_r(NULL, " ", &saveptr);
			if (p == NULL)
			{
				close_internal(fd_m);
				return -ENOENT;
			}
		}

		mount_path = p;

		break;
	}

	close_internal(fd_m);

	if (mount_path == NULL)
		return -ENOENT;
	
	// We have the path of the mount, lets get a file descriptor
	fd_m = sys_open_nocancel(mount_path, BSD_O_RDONLY, 0);
	if (fd_m < 0)
		return fd_m;
	
	// And now, finally, open the file by handle relative to the mount
	int ret = LINUX_SYSCALL(__NR_open_by_handle_at, fd_m, &ref->fh, linux_flags);

	close_internal(fd_m);

	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}
