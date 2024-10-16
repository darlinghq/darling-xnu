#include "close.h"

// #include "../mach/lkm.h"

#include <emulation/common/simple.h>
#include <emulation/linux_api/linux_syscall.h>
#include <emulation/xnu_syscall/shared/bsdthread/cancelable.h>
#include <emulation/xnu_syscall/shared/guarded/table.h>

extern int kqueue_close(int kq);
extern void kqueue_closed_fd(int fd);

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

	if (kqueue_close(fd)) {
		// this FD belongs to libkqueue and it will take care of closing it
		return 0;
	}

	ret = LINUX_SYSCALL1(__NR_close, fd);
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);
	else
		kqueue_closed_fd(fd);

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
