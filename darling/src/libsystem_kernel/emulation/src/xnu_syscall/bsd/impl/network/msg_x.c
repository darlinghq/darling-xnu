#include <darling/emulation/xnu_syscall/bsd/impl/network/msg_x.h>

#include <sys/errno.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/network/duct.h>
#include <darling/emulation/xnu_syscall/bsd/helper/bsdthread/cancelable.h>
#include <darling/emulation/xnu_syscall/bsd/impl/network/recvmsg.h>
#include <darling/emulation/xnu_syscall/bsd/impl/network/sendmsg.h>

// The batched calls are implemented by one recvmsg/sendmsg per entry. Darwin
// documents MSG_DONTWAIT as the only supported flag for both calls, and
// recvmsg_x must stop as soon as a datagram is truncated. The count returned
// is the number of messages handled, not the number of bytes.

long sys_recvmsg_x(int socket, struct bsd_msghdr_x* msgp, unsigned int cnt, int flags)
{
	unsigned int i;

	CANCELATION_POINT();
	if (msgp == NULL || cnt == 0 || (flags & ~BSD_MSG_DONTWAIT))
		return -EINVAL;

	for (i = 0; i < cnt; ++i)
	{
		long ret = sys_recvmsg_nocancel(socket, &msgp[i].hdr, i == 0 ? flags : (flags | BSD_MSG_DONTWAIT));
		if (ret < 0)
		{
			if (i == 0)
				return ret;
			break;
		}

		// sys_recvmsg_nocancel() has already converted msg_flags; preserve
		// flags such as MSG_EOR and only use MSG_TRUNC to stop the batch.
		msgp[i].msg_datalen = (size_t) ret;
		if (msgp[i].hdr.msg_flags & BSD_MSG_TRUNC)
			return i + 1;
	}

	return i;
}

long sys_sendmsg_x(int socket, struct bsd_msghdr_x* msgp, unsigned int cnt, int flags)
{
	unsigned int i;

	CANCELATION_POINT();
	if (msgp == NULL || cnt == 0 || (flags & ~BSD_MSG_DONTWAIT))
		return -EINVAL;

	for (i = 0; i < cnt; ++i)
	{
		long ret = sys_sendmsg_nocancel(socket, &msgp[i].hdr, flags);
		if (ret < 0)
		{
			if (i == 0)
				return ret;
			break;
		}
	}

	return i;
}
