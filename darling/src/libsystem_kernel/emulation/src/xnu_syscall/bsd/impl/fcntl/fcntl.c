#include <emulation/xnu_syscall/bsd/impl/fcntl/fcntl.h>

#include <emulation/common/simple.h>
#include <emulation/conversion/methods/errno.h>
#include <emulation/conversion/methods/fd/fcntl.h>
#include <emulation/linux_api/linux_syscall.h>
#include <emulation/xnu_syscall/bsd/impl/fcntl/open.h>
#include <emulation/xnu_syscall/shared/fdpath.h>
#include <emulation/xnu_syscall/shared/bsdthread/cancelable.h>

#ifndef MAXPATHLEN
#	define MAXPATHLEN 1024
#endif

int oflags_bsd_to_linux(int flags);
int oflags_linux_to_bsd(int flags);

extern int sprintf(char *str, const char *format, ...);
long sys_readlink(const char* path, char* buf, unsigned long bsize);

extern void kqueue_dup(int oldfd, int newfd);

long sys_fcntl(int fd, int cmd, long arg)
{
	CANCELATION_POINT();
	return sys_fcntl_nocancel(fd, cmd, arg);
}

long sys_fcntl_nocancel(int fd, int cmd, long arg)
{
	int ret, linux_cmd = cmd_bsd_to_linux(cmd);
	long original_arg = arg;

	switch (cmd)
	{
		case BSD_F_CHECK_LV:
			return 0;
		case BSD_F_SETFD:
			if (arg & ~BSD_FD_CLOEXEC)
				return -BSD_EINVAL;
			break;
		case BSD_F_SETFL:
			arg = oflags_bsd_to_linux(arg);
			break;
		case BSD_F_GETPATH:
		{
			ret = fdpath(fd, arg, MAXPATHLEN);
			if (ret < 0)
				return errno_linux_to_bsd(ret);

			return 0;
		}
		case BSD_F_SETLK:
		case BSD_F_SETLKW:
		case BSD_F_GETLK: {
			struct bsd_flock* bsd_struct = arg;
			struct linux_flock* linux_struct = __builtin_alloca(sizeof(struct linux_flock));
			linux_struct->l_type = flock_type_bsd_to_linux(bsd_struct->l_type);
			linux_struct->l_whence = bsd_struct->l_whence;
			linux_struct->l_start = bsd_struct->l_start;
			linux_struct->l_len = bsd_struct->l_len;
			linux_struct->l_pid = bsd_struct->l_pid;
			arg = linux_struct;
		} break;
		case BSD_F_FULLFSYNC: {
			ret = LINUX_SYSCALL1(__NR_fsync, fd);
			if (ret < 0)
				ret = errno_linux_to_bsd(ret);
			return 0;
		};
		// TODO: implement remaining commands
		default:
			if (linux_cmd == -1)
				return -BSD_EINVAL;
	}

#ifdef __NR_fcntl64
	ret = LINUX_SYSCALL(__NR_fcntl64, fd, linux_cmd, arg);
#else
	ret = LINUX_SYSCALL(__NR_fcntl, fd, linux_cmd, arg);
#endif
	if (ret < 0)
	{
		ret = errno_linux_to_bsd(ret);
		return ret;
	}

	switch (cmd)
	{
		case BSD_F_GETFL:
			ret = oflags_linux_to_bsd(ret);
			break;
		case BSD_F_SETLK:
		case BSD_F_SETLKW:
		case BSD_F_GETLK: {
			struct bsd_flock* bsd_struct = original_arg;
			struct linux_flock* linux_struct = arg;
			bsd_struct->l_type = flock_type_linux_to_bsd(linux_struct->l_type);
			bsd_struct->l_whence = linux_struct->l_whence;
			bsd_struct->l_start = linux_struct->l_start;
			bsd_struct->l_len = linux_struct->l_len;
			bsd_struct->l_pid = linux_struct->l_pid;
		} break;
		case BSD_F_DUPFD:
		case BSD_F_DUPFD_CLOEXEC:
			kqueue_dup(fd, ret);
			break;
	}

	return ret;
}

