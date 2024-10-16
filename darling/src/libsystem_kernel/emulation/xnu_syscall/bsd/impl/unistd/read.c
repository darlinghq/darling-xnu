#include "read.h"

#include <emulation/conversion/errno.h>
#include <emulation/linux_api/linux_syscall.h>
#include <emulation/linux_api/c_api/ioctl.h>
#include <emulation/xnu_syscall/shared/bsdthread/cancelable.h>

#define LINUX_TCGETA 0x5405

long sys_read(int fd, void* mem, int len)
{
	CANCELATION_POINT();
	return sys_read_nocancel(fd, mem, len);
}

long sys_read_nocancel(int fd, void* mem, int len)
{
	int ret;

	ret = LINUX_SYSCALL3(__NR_read, fd, mem, len);
	if (ret < 0)
	{
		// When the slave side of a PTY is closed, Linux produces -EIO on the master side.
		// macOS produces an EOF.
		if (ret == -LINUX_EIO)
		{
			// Not making this static makes everything go boom. Do we have a too small stack somewhere?
			static char dummy[18];
			if (__real_ioctl(fd, LINUX_TCGETA, dummy) == 0) // "isatty(fd)"
				return 0;
		}
		return errno_linux_to_bsd(ret);
	}

	return ret;
}

