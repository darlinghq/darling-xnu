#include <darling/emulation/xnu_syscall/bsd/impl/unistd/write.h>

#include <stddef.h>
#include <sys/errno.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/xnu_syscall/bsd/helper/bsdthread/cancelable.h>
#include <darling/emulation/xnu_syscall/bsd/helper/bpf/bpf_device.h>
#include <darling/emulation/xnu_syscall/bsd/helper/network/nosigpipe.h>
#include <darling/emulation/xnu_syscall/bsd/impl/network/sendto.h>

static long sys_write_common(int fd, const void* mem, int len, bool cancelable);

long sys_write(int fd, const void* mem, int len)
{
	CANCELATION_POINT();
	return sys_write_common(fd, mem, len, true);
}

__attribute__((visibility("default")))
long __write_for_xtrace(int fd, const void* mem, int len)
{
	return sys_write_nocancel(fd, mem, len);
}


long sys_write_nocancel(int fd, const void* mem, int len)
{
	return sys_write_common(fd, mem, len, false);
}

static long sys_write_common(int fd, const void* mem, int len, bool cancelable)
{
	int ret;
	long bpf_ret;

	if (bpf_device_fd_count && bpf_device_write(fd, mem, len, &bpf_ret))
		return bpf_ret;

	if (nosigpipe_get(fd))
	{
		// A socket with SO_NOSIGPIPE: send() is write() plus the flag that
		// keeps SIGPIPE from being raised.
		long sent = cancelable ? sys_sendto(fd, mem, len, 0, NULL, 0)
			: sys_sendto_nocancel(fd, mem, len, 0, NULL, 0);
		if (sent != -ENOTSOCK)
			return sent;
		// The option was left behind by a descriptor that is no socket.
		nosigpipe_close(fd);
	}

	ret = LINUX_SYSCALL_C(cancelable, __NR_write, fd, mem, len);
	if (ret < 0)
		return errno_linux_to_bsd(ret);

	return ret;
}
