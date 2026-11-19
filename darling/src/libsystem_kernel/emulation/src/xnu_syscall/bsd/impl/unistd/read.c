#include <darling/emulation/xnu_syscall/bsd/impl/unistd/read.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/xnu_syscall/bsd/helper/bsdthread/cancelable.h>
#include <darling/emulation/linux_premigration/misc/ioctl.h>
#include <darling/emulation/conversion/duct_errno.h>
#include <darling/emulation/conversion/ioctl/termios.h>
#include <darling/emulation/xnu_syscall/bsd/helper/bpf/bpf_device.h>

#define LINUX_TCGETA 0x5405

static long sys_read_common(int fd, void* mem, int len, bool cancelable);

// Linux reports the closed other side of a pseudo-terminal with EIO where macOS reads an EOF:
// on the master once the slave is closed, and on the slave once the master is closed. A read
// from the controlling terminal by a background process group that ignores or blocks SIGTTIN
// fails with EIO on both systems, so that one stays an error.
static bool eio_means_eof(int fd)
{
	unsigned int ptyno;
	if (__real_ioctl(fd, LINUX_TIOCGPTN, &ptyno) == 0) // only a master answers TIOCGPTN
		return true;

	int pgrp;
	if (__real_ioctl(fd, LINUX_TIOCGPGRP, &pgrp) == 0) // the caller's controlling terminal
		return pgrp == LINUX_SYSCALL(__NR_getpgid, 0);

	// Not making this static makes everything go boom. Do we have a too small stack somewhere?
	static char termio[18];
	return __real_ioctl(fd, LINUX_TCGETA, termio) == 0; // any other terminal
}

long sys_read(int fd, void* mem, int len)
{
	CANCELATION_POINT();
	return sys_read_common(fd, mem, len, true);
}

long sys_read_nocancel(int fd, void* mem, int len)
{
	return sys_read_common(fd, mem, len, false);
}

static long sys_read_common(int fd, void* mem, int len, bool cancelable)
{
	int ret;
	long bpf_ret;

	if (bpf_device_fd_count && bpf_device_read(fd, mem, len, &bpf_ret))
		return bpf_ret;

	ret = LINUX_SYSCALL_C(cancelable, __NR_read, fd, mem, len);
	if (ret < 0)
	{
		if (ret == -LINUX_EIO && eio_means_eof(fd))
			return 0;
		return errno_linux_to_bsd(ret);
	}

	return ret;
}
