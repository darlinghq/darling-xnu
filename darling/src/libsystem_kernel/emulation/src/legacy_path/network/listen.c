#include "listen.h"
#include "../base.h"
#include "../errno.h"
#include <linux-syscalls/linux.h>

void __attribute__((weak)) __attribute__((visibility("default"))) __darling_kqueue_register_listen(int fd) { }

long sys_listen(int fd, int backlog)
{
	int ret;

#ifdef __NR_socketcall
	ret = LINUX_SYSCALL(__NR_socketcall, LINUX_SYS_LISTEN,
			((long[6]) { fd, backlog }));
#else
	ret = LINUX_SYSCALL(__NR_listen, fd, backlog);
#endif

	if (ret < 0)
		ret = errno_linux_to_bsd(ret);
	else
		__darling_kqueue_register_listen(fd);

	return ret;
}

