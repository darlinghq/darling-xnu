#include <darling/emulation/xnu_syscall/bsd/impl/unistd/close.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/xnu_syscall/bsd/helper/bsdthread/cancelable.h>
#include <darling/emulation/other/mach/lkm.h>
#include <darling/emulation/common/simple.h>
#include <darling/emulation/common/guarded/table.h>
#include <_libkernel_init.h>

extern _libkernel_functions_t _libkernel_functions;

long sys_close(int fd)
{
	CANCELATION_POINT();
	return sys_close_nocancel(fd);
}

long sys_close_nocancel(int fd)
{
	int ret;

	if (guard_table_check(fd, guard_flag_prevent_close)) {
		// we should crash, actually.
		// for now, let's silently (as far as the caller is concerned) ignore it.
		__simple_kprintf("*** Someone tried to close a guarded FD (%d) via close! ***", fd);
		return 0;
	}

	if (_libkernel_functions && _libkernel_functions->kqueue_close) {
		if (_libkernel_functions->kqueue_close(fd)) {
			// this FD belongs to libkqueue and it will take care of closing it
			return 0;
		}
	}

	ret = LINUX_SYSCALL1(__NR_close, fd);
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);
	else {
		if (_libkernel_functions && _libkernel_functions->kqueue_closed_fd)
			_libkernel_functions->kqueue_closed_fd(fd);
	}

	return ret;
}

long close_internal(int fd)
{
	int ret;

	ret = LINUX_SYSCALL1(__NR_close, fd);
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}

// Special variant for libkqueue to avoid recursion into kqueue_close()/kqueue_closed_fd()
__attribute__((visibility("default")))
long __close_for_kqueue(int fd)
{
	return close_internal(fd);
}
