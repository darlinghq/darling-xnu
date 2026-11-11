#include <darling/emulation/xnu_syscall/bsd/impl/network/sendmsg.h>

#include <stddef.h>

#include <darling/emulation/common/base.h>
#include <sys/errno.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/conversion/network/cmsg.h>
#include <darling/emulation/conversion/network/duct.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/xnu_syscall/bsd/helper/network/duct.h>
#include <darling/emulation/xnu_syscall/bsd/helper/bsdthread/cancelable.h>

extern void *malloc(__SIZE_TYPE__ size);
extern void free(void* ptr);

// Control buffers up to this size are converted on the stack.
#define SENDMSG_STACK_CONTROL 256

long sys_sendmsg(int socket, const struct bsd_msghdr* msg, int flags)
{
	CANCELATION_POINT();
	return sys_sendmsg_nocancel(socket, msg, flags);
}

long sys_sendmsg_nocancel(int socket, const struct bsd_msghdr* msg, int flags)
{
	int ret, linux_flags;
	struct linux_msghdr lmsg;
	void* heap_control = NULL;
	char stack_control[SENDMSG_STACK_CONTROL] __attribute__((aligned(sizeof(size_t))));

	if (msg->msg_name != NULL && msg->msg_namelen > 0)
	{
		struct sockaddr_fixup* saddr;

		// Same limit as sendto() and connect(): the address is copied to the
		// stack.
		if (msg->msg_namelen > 512)
			return -EINVAL;

		saddr = __builtin_alloca(sockaddr_fixup_size_from_bsd(msg->msg_name, msg->msg_namelen));
		ret = sockaddr_fixup_from_bsd(saddr, msg->msg_name, msg->msg_namelen);
		if (ret < 0)
			return ret;

		lmsg.msg_name = saddr;
		lmsg.msg_namelen = ret;
	}
	else
	{
		// A length without an address means no address, as on macOS.
		lmsg.msg_name = NULL;
		lmsg.msg_namelen = 0;
	}

	lmsg.msg_iov = msg->msg_iov;
	lmsg.msg_iovlen = msg->msg_iovlen;
	lmsg.msg_control = NULL;
	lmsg.msg_controllen = 0;

	if (msg->msg_control != NULL)
	{
		long size = cmsg_bsd_to_linux_size(msg->msg_control, msg->msg_controllen);
		if (size < 0)
			return size;

		if (size > 0)
		{
			void* buf = stack_control;

			if (size > sizeof(stack_control))
			{
				heap_control = malloc(size);
				if (heap_control == NULL)
					return -ENOMEM;
				buf = heap_control;
			}

			lmsg.msg_controllen = cmsg_bsd_to_linux(msg->msg_control, msg->msg_controllen, buf);
			lmsg.msg_control = buf;
		}
	}

	lmsg.msg_flags = 0; // ignored

	linux_flags = msgflags_bsd_to_linux(flags);

#ifdef __NR_socketcall
	ret = LINUX_SYSCALL(__NR_socketcall, LINUX_SYS_SENDMSG,
			((long[6]) { socket, &lmsg, linux_flags }));
#else
	ret = LINUX_SYSCALL(__NR_sendmsg, socket, &lmsg, linux_flags);
#endif

	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	if (heap_control != NULL)
		free(heap_control);

	return ret;
}
