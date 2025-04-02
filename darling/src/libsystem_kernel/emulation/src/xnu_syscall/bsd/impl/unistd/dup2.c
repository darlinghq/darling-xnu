#include <emulation/xnu_syscall/bsd/impl/unistd/dup2.h>

#include <emulation/common/simple.h>
#include <emulation/linux_api/linux_syscall.h>
#include <emulation/xnu_syscall/shared/guarded/table.h>

extern void kqueue_dup(int oldfd, int newfd);

long sys_dup2(int fd_from, int fd_to)
{
	int ret;

	if (guard_table_check(fd_to, guard_flag_prevent_close)) {
		__simple_kprintf("*** Someone tried to close a guarded FD (%d) via dup2! ***", fd_to);
		__simple_abort();
	}

	#if defined(__NR_dup2)
		ret = LINUX_SYSCALL2(__NR_dup2, fd_from, fd_to);
	#else		
		// It's not clear if 0 is offically a valid flag argument.
		// But we don't really have a choice.
		ret = LINUX_SYSCALL(__NR_dup3, fd_from, fd_to, 0);

		if (ret == LINUX_EINVAL && fd_from == fd_to)
			ret = fd_to;
	#endif
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);
	else
		kqueue_dup(fd_from, fd_to);

	return ret;
}

