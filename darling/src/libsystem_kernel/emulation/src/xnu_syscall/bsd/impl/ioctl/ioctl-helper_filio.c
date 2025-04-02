#include <emulation/xnu_syscall/bsd/impl/ioctl/ioctl-helper_filio.h>

#include <emulation/common/simple.h>
#include <emulation/conversion/methods/errno.h>
#include <emulation/conversion/variables/bsd/sys/conf.h>
#include <emulation/conversion/variables/bsd/sys/filio.h>
#include <emulation/conversion/variables/linux/asm/sockios.h>
#include <emulation/linux_api/c_api/ioctl.h>
#include <emulation/xnu_syscall/bsd/impl/ioctl/ioctl.h>
#include <emulation/xnu_syscall/shared/fdpath.h>

#include <stdbool.h>


extern int sprintf(char* buf, const char* fmt, ...);
extern int strncmp(const char *, const char *, __SIZE_TYPE__);
extern __SIZE_TYPE__ strlen(const char*);
extern void* memmove(void*, void*, __SIZE_TYPE__);

static bool get_fd_path(int fd, char* buf, size_t len)
{
	int ret;

	ret = fdpath(fd, buf, len);
	if (ret < 0)
		return false;

	if (strncmp(buf, "/Volumes/SystemRoot/dev/", 24) == 0)
	{
		// Remove /Volumes/SystemRoot from the start
		memmove(buf, buf + 19, strlen(buf + 19) + 1);
	}
	return true;
}


int handle_filio(int fd, int cmd, void* arg, int* retval)
{
	switch (cmd)
	{
		case BSD_FIODTYPE:
		{
			char orig_path[256];
			int* iarg = (int*) arg;

			if (iarg == NULL)
			{
				*retval = -BSD_EFAULT;
				return IOCTL_HANDLED;
			}

			if (!get_fd_path(fd, orig_path, sizeof(orig_path)))
			{
				*retval = -BSD_EBADF;
				return IOCTL_HANDLED;
			}
			__simple_kprintf("dtype for fd %d -> %s\n", fd, orig_path);

			*retval = 0;
			if (strncmp(orig_path, "/dev/pts", 8) == 0
					|| strncmp(orig_path, "/dev/tty", 8) == 0)
			{
				*iarg = BSD_D_TTY;
			}
			else if (strncmp(orig_path, "/dev/st", 7) == 0
					|| strncmp(orig_path, "/dev/nst", 8) == 0)
			{
				*iarg = BSD_D_TAPE;
			}
			else
			{
				*iarg = BSD_D_DISK;
			}
			return IOCTL_HANDLED;
		}
		case BSD_FIONBIO:
		{
			*retval = __real_ioctl(fd, 0x5421, arg);
			if (*retval < 0)
				*retval = errno_linux_to_bsd(*retval);
			
			return IOCTL_HANDLED;
		}
		case BSD_FIOASYNC:
		{
			*retval = __real_ioctl(fd, 0x5452, arg);
			if (*retval < 0)
				*retval = errno_linux_to_bsd(*retval);
			
			return IOCTL_HANDLED;
		}
		case BSD_FIOCLEX:
		{
			*retval = __real_ioctl(fd, 0x5451, arg);
			if (*retval < 0)
				*retval = errno_linux_to_bsd(*retval);
			
			return IOCTL_HANDLED;
		}
		case BSD_FIONCLEX:
		{
			*retval = __real_ioctl(fd, 0x5450, arg);
			if (*retval < 0)
				*retval = errno_linux_to_bsd(*retval);
			
			return IOCTL_HANDLED;
		}
		case BSD_FIONREAD:
		{
			*retval = __real_ioctl(fd, 0x541B, arg);
			if (*retval < 0)
				*retval = errno_linux_to_bsd(*retval);
			
			return IOCTL_HANDLED;
		}
		case BSD_FIOGETOWN:
		{
			*retval = __real_ioctl(fd, LINUX_FIOGETOWN, arg);
			if (*retval < 0)
				*retval = errno_linux_to_bsd(*retval);
			
			return IOCTL_HANDLED;
		}
		case BSD_FIOSETOWN:
		{
			*retval = __real_ioctl(fd, LINUX_FIOSETOWN, arg);
			if (*retval < 0)
				*retval = errno_linux_to_bsd(*retval);
			
			return IOCTL_HANDLED;
		}
		/*
		case BSD_FIOASYNC:
		{
			int flags;
			int* iarg = (int*) arg;
			
			if (!iarg)
				*retval = -EINVAL;
			
			*retval = sys_fcntl(fd, F_GETFL, 0);
			if (*retval < 0)
				return IOCTL_HANDLED;
			
			flags = *retval;
			
			if (*iarg)
				flags |= BSD_O_ASYNC;
			else
				flags &= ~BSD_O_ASYNC;
			
			*retval = sys_fcntl(fd, F_SETFL, flags);
			
			return IOCTL_HANDLED;
		}
		*/
		default:
			return IOCTL_PASS;
	}
}

