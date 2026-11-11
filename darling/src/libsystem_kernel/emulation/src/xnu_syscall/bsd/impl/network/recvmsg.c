#include <darling/emulation/xnu_syscall/bsd/impl/network/recvmsg.h>

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

// Control buffers up to this size are received on the stack.
#define RECVMSG_STACK_CONTROL 512

long sys_recvmsg(int socket, struct bsd_msghdr* msg, int flags)
{
	CANCELATION_POINT();
	return sys_recvmsg_nocancel(socket, msg, flags);
}

long sys_recvmsg_nocancel(int socket, struct bsd_msghdr* msg, int flags)
{
	int ret, linux_flags;
	struct linux_msghdr lmsg;
	void* heap_control = NULL;
	char stack_control[RECVMSG_STACK_CONTROL] __attribute__((aligned(sizeof(size_t))));
	size_t lcontrollen;

	if (msg->msg_name != NULL)
	{
		lmsg.msg_name = msg->msg_name;
		lmsg.msg_namelen = msg->msg_namelen;
	}
	else
	{
		lmsg.msg_name = NULL;
		lmsg.msg_namelen = 0;
	}

	lmsg.msg_iov = msg->msg_iov;
	lmsg.msg_iovlen = msg->msg_iovlen;

	// The Darwin buffer cannot be handed to Linux even where the layouts
	// agree (i386): levels and types are numbered differently. Receive into
	// a buffer of our own and convert. Without room for a single header,
	// Linux gets none, sets MSG_CTRUNC itself and closes any descriptors.
	lcontrollen = (msg->msg_control != NULL) ? cmsg_linux_space_for_bsd(msg->msg_controllen) : 0;
	if (lcontrollen > sizeof(stack_control))
	{
		heap_control = malloc(lcontrollen);
		if (heap_control == NULL)
			return -ENOMEM;
		lmsg.msg_control = heap_control;
	}
	else
		lmsg.msg_control = (lcontrollen > 0) ? stack_control : NULL;
	lmsg.msg_controllen = lcontrollen;

	lmsg.msg_flags = 0; // set on return

	linux_flags = msgflags_bsd_to_linux(flags);

#ifdef __NR_socketcall
	ret = LINUX_SYSCALL(__NR_socketcall, LINUX_SYS_RECVMSG,
			((long[6]) { socket, &lmsg, linux_flags }));
#else
	ret = LINUX_SYSCALL(__NR_recvmsg, socket, &lmsg, linux_flags);
#endif

	if (ret < 0)
		ret = errno_linux_to_bsd(ret);
	else
	{
		int bsd_flags = msgflags_linux_to_bsd(lmsg.msg_flags);

		if (msg->msg_control != NULL)
		{
			int truncated = 0;

			msg->msg_controllen = cmsg_linux_to_bsd(lmsg.msg_control, lmsg.msg_control ? lmsg.msg_controllen : 0,
					msg->msg_control, msg->msg_controllen, &truncated);
			if (truncated)
				bsd_flags |= BSD_MSG_CTRUNC;
		}
		else
			msg->msg_controllen = 0;

		msg->msg_flags = bsd_flags;

		if (msg->msg_name != NULL)
		{
			int namelen = sockaddr_fixup_from_linux((struct sockaddr_fixup*) msg->msg_name,
					msg->msg_name, lmsg.msg_namelen, msg->msg_namelen);
			// The data (and any descriptors) has been received by now;
			// failing the call would lose it, so report no address instead.
			msg->msg_namelen = (namelen < 0) ? 0 : namelen;
		}
		else
			msg->msg_namelen = 0;
	}

	if (heap_control != NULL)
		free(heap_control);

	return ret;
}
