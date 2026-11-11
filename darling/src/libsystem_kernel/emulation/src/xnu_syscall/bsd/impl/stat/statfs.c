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
#include <darling/emulation/xnu_syscall/bsd/impl/stat/mount_table.h>

#undef strlcpy

extern char *strtok_r(char *str, const char *delim, char **saveptr);
extern __SIZE_TYPE__ strlcpy(char* dst, const char* src, __SIZE_TYPE__ size);
extern __SIZE_TYPE__ strlen(const char* str);
extern int strncmp(const char* s1, const char* s2, __SIZE_TYPE__ len);

long sys_statfs(const char* path, struct bsd_statfs* buf)
{
#ifdef __x86_64__
	return sys_statfs64(path, (struct bsd_statfs64*) buf);
#else
#	warning Not implemented
	return 0;
#endif
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

#ifdef __NR_statfs64
	ret = LINUX_SYSCALL2(__NR_statfs64, vc.path, &lbuf);
#else
	ret = LINUX_SYSCALL2(__NR_statfs, vc.path, &lbuf);
#endif

	if (ret < 0)
		return errno_linux_to_bsd(ret);

	statfs_linux_to_bsd64(&lbuf, buf);

	return mount_table_lookup(vc.path,
		buf->f_mntonname, sizeof(buf->f_mntonname),
		buf->f_fstypename, sizeof(buf->f_fstypename),
		buf->f_mntfromname, sizeof(buf->f_mntfromname));
}
