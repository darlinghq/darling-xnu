#include <darling/emulation/xnu_syscall/bsd/impl/stat/statfs.h>

#include <stddef.h>
#include <sys/fcntl.h>

#include <darling/emulation/conversion/stat/common.h>
#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/common/simple.h>
#include <darling/emulation/xnu_syscall/bsd/impl/fcntl/open.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/close.h>
#include <darling/emulation/linux_premigration/vchroot_expand.h>
#include <darling/emulation/common/bsdthread/per_thread_wd.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/xnu_syscall/bsd/helper/stat/guest_mount.h>
#include <darling/emulation/xnu_syscall/bsd/helper/stat/linux_statfs.h>

#undef strlcpy

extern char *strtok_r(char *str, const char *delim, char **saveptr);
extern __SIZE_TYPE__ strlcpy(char* dst, const char* src, __SIZE_TYPE__ size);
extern __SIZE_TYPE__ strlen(const char* str);
extern int strncmp(const char* s1, const char* s2, __SIZE_TYPE__ len);

long sys_statfs(const char* path, struct bsd_statfs* buf)
{
	// the legacy structure (every architecture has one, with the long fields of its own width)
	struct bsd_statfs64 wide;
	long ret = sys_statfs64(path, &wide);

	if (ret < 0)
		return ret;
	statfs_bsd64_to_bsd(&wide, buf);
	return 0;
}

long sys_statfs64(const char* path, struct bsd_statfs64* buf)
{
	int ret;
	struct linux_statfs64 lbuf;

	struct vchroot_expand_args vc;
	vc.flags = VCHROOT_FOLLOW;
	vc.dfd = get_perthread_wd();

	strcpy(vc.path, path);
	ret = vchroot_expand(&vc);
	if (ret < 0)
		return errno_linux_to_bsd(ret);

	ret = linux_statfs_path(vc.path, &lbuf);

	if (ret < 0)
		return errno_linux_to_bsd(ret);

	statfs_linux_to_bsd64(&lbuf, buf);

	return guest_mount_describe(vc.path, buf);
}
