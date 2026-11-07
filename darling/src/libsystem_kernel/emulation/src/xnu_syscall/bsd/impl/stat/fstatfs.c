#include <darling/emulation/xnu_syscall/bsd/impl/stat/fstatfs.h>

#include <stddef.h>
#include <sys/fcntl.h>

#include <darling/emulation/conversion/stat/common.h>
#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/common/simple.h>
#include <darling/emulation/xnu_syscall/bsd/impl/fcntl/open.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/close.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/readlink.h>
#include <darling/emulation/xnu_syscall/bsd/helper/misc/fdpath.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/xnu_syscall/bsd/impl/stat/mount_table.h>

extern char *strtok_r(char *str, const char *delim, char **saveptr);
extern __SIZE_TYPE__ strlcpy(char* dst, const char* src, __SIZE_TYPE__ size);
extern __SIZE_TYPE__ strlen(const char* str);
extern int strncmp(const char* s1, const char* s2, __SIZE_TYPE__ len);
extern int sprintf(char *str, const char *format, ...);

long sys_fstatfs(int fd, struct bsd_statfs* buf)
{
#ifdef __x86_64__
	return sys_fstatfs64(fd, (struct bsd_statfs64*) buf);
#else
#	warning Not implemented
	return 0;
#endif
}

long sys_fstatfs64(int fd, struct bsd_statfs64* buf)
{
	int ret;
	struct linux_statfs64 lbuf;
	char path[1024];

#ifdef __NR_fstatfs64
	ret = LINUX_SYSCALL3(__NR_fstatfs64, fd, sizeof(lbuf), &lbuf);
#else
	ret = LINUX_SYSCALL2(__NR_fstatfs, fd, &lbuf);
#endif

	if (ret < 0)
		return errno_linux_to_bsd(ret);

	statfs_linux_to_bsd64(&lbuf, buf);

	ret = fdpath(fd, path, sizeof(path));

	if (ret < 0)
		return ret;

	buf->f_iosize = 512;

	return mount_table_lookup(path,
		buf->f_mntonname, sizeof(buf->f_mntonname),
		buf->f_fstypename, sizeof(buf->f_fstypename),
		buf->f_mntfromname, sizeof(buf->f_mntfromname));
}
