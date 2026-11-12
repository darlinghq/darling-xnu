#include <darling/emulation/xnu_syscall/bsd/impl/network/msg_x.h>

#include <sys/errno.h>

#include <darling/emulation/conversion/network/duct.h>
#include <darling/emulation/xnu_syscall/bsd/impl/network/recvmsg.h>
#include <darling/emulation/xnu_syscall/bsd/impl/network/sendmsg.h>
#include <darling/emulation/xnu_syscall/bsd/helper/bsdthread/cancelable.h>

// recvmsg_x / sendmsg_x as XNU's soreceive_m_list / sosend_list answer them, one recvmsg / sendmsg per
// entry: receive up to cnt datagrams (only the first may block: the rest are taken while they are already queued),
// each entry's msg_datalen set to its datagram's length; send up to cnt. Both return how many entries were done, or
// the error when none was.

long sys_recvmsg_x(int socket, struct bsd_msghdr_x* msgp, unsigned int cnt, int flags)
{
	unsigned int i;
	CANCELATION_POINT();
	if (msgp == NULL || cnt == 0)
		return -EINVAL;
	for (i = 0; i < cnt; i++)
	{
		long ret = sys_recvmsg_nocancel(socket, &msgp[i].hdr, i == 0 ? flags : (flags | BSD_MSG_DONTWAIT));
		if (ret < 0)
		{
			if (i == 0)
				return ret;
			break;  // nothing more queued (EWOULDBLOCK) or an error the next call reports
		}
		msgp[i].msg_datalen = (size_t) ret;
	}
	return i;
}

long sys_sendmsg_x(int socket, struct bsd_msghdr_x* msgp, unsigned int cnt, int flags)
{
	unsigned int i;
	CANCELATION_POINT();
	if (msgp == NULL || cnt == 0)
		return -EINVAL;
	for (i = 0; i < cnt; i++)
	{
		long ret = sys_sendmsg_nocancel(socket, &msgp[i].hdr, flags);
		if (ret < 0)
		{
			if (i == 0)
				return ret;
			break;
		}
		msgp[i].msg_datalen = (size_t) ret;
	}
	return i;
}
