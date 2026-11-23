#include <darling/emulation/xnu_syscall/bsd/impl/unistd/writev.h>

#include <sys/errno.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/xnu_syscall/bsd/helper/bsdthread/cancelable.h>
#include <darling/emulation/xnu_syscall/bsd/helper/network/nosigpipe.h>
#include <darling/emulation/xnu_syscall/bsd/impl/network/sendmsg.h>

static long sys_writev_common(int fd, struct iovec* iovp, unsigned int len, bool cancelable);

long sys_writev(int fd, struct iovec* iovp, unsigned int len)
{
	CANCELATION_POINT();
	return sys_writev_common(fd, iovp, len, true);
}

long sys_writev_nocancel(int fd, struct iovec* iovp, unsigned int len)
{
	return sys_writev_common(fd, iovp, len, false);
}

static long sys_writev_common(int fd, struct iovec* iovp, unsigned int len, bool cancelable)
{
	int ret;

	if (nosigpipe_get(fd))
	{
		// See sys_write_common().
		struct bsd_msghdr msg = { .msg_iov = iovp, .msg_iovlen = len };
		long sent = cancelable ? sys_sendmsg(fd, &msg, 0) : sys_sendmsg_nocancel(fd, &msg, 0);
		if (sent != -ENOTSOCK)
			return sent;
		nosigpipe_close(fd);
	}

	ret = LINUX_SYSCALL_C(cancelable, __NR_writev, fd, iovp, len);
	if (ret < 0)
		return errno_linux_to_bsd(ret);

	return ret;
}
