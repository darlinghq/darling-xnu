#include <darling/emulation/xnu_syscall/bsd/impl/network/socket.h>

#include <sys/socket.h>
#include <sys/errno.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/conversion/network/duct.h>
#include <darling/emulation/conversion/network/socket.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/xnu_syscall/bsd/helper/network/duct.h>

long sys_socket(int domain, int type, int protocol)
{
	int ret;
	int linux_domain;

	switch (domain)
	{
		case PF_LOCAL:
			linux_domain = LINUX_PF_LOCAL; break;
		case PF_INET:
			linux_domain = LINUX_PF_INET; break;
		case PF_IPX:
			linux_domain = LINUX_PF_IPX; break;
		case PF_INET6:
			linux_domain = LINUX_PF_INET6; break;
		default:
			return -EINVAL;
	}

#ifdef __NR_socketcall
	ret = LINUX_SYSCALL(__NR_socketcall, LINUX_SYS_SOCKET,
			((long[6]) { linux_domain, type, protocol }));
#else
	ret = LINUX_SYSCALL(__NR_socket, linux_domain, type, protocol);
#endif

	if (ret == -24 /*-EMFILE*/) {
		// DARLING arm64 fix: per-thread RPC sockets occupy fd 1021/1022/1023;
		// combined with the initial dyld-mmap'd dylib fds, a fresh Darwin
		// process already has all 1024 of its soft-limited fd slots taken,
		// so the very first user socket() call hits EMFILE. Raise the soft
		// limit to the hard limit (~512k in our container) and retry.
		struct { unsigned long cur; unsigned long max; } rlim = {0, 0};
		LINUX_SYSCALL(__NR_prlimit64, 0, 7 /*RLIMIT_NOFILE*/, 0, &rlim);
		struct { unsigned long cur; unsigned long max; } newrlim = { rlim.max, rlim.max };
		LINUX_SYSCALL(__NR_prlimit64, 0, 7, &newrlim, 0);

#ifdef __NR_socketcall
		ret = LINUX_SYSCALL(__NR_socketcall, LINUX_SYS_SOCKET, ((long[6]){ linux_domain, type, protocol }));
#else
		ret = LINUX_SYSCALL(__NR_socket, linux_domain, type, protocol);
#endif
	}

	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}
