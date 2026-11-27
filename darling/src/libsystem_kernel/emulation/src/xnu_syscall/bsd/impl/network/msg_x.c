#include <darling/emulation/xnu_syscall/bsd/impl/network/msg_x.h>

#include <sys/errno.h>

#include <darling/emulation/conversion/network/duct.h>
#include <darling/emulation/xnu_syscall/bsd/impl/network/recvmsg.h>
#include <darling/emulation/xnu_syscall/bsd/impl/network/sendmsg.h>
#include <darling/emulation/xnu_syscall/bsd/helper/bsdthread/cancelable.h>

// recvmsg_x / sendmsg_x, one recvmsg / sendmsg per entry, as observed on macOS 10.14 and 26 (see
// tests/libsystem_kernel-msg-x.c):
//  - recvmsg_x receives up to cnt datagrams; only the first may block, the rest are taken while they are already
//    queued. Each entry's msg_datalen is its datagram's length. A datagram that does not fit (MSG_TRUNC) ends the
//    batch. The return value is the number of entries received, or the error when there was none.
//  - sendmsg_x sends up to cnt datagrams and returns how many were sent, or the error when none was. msg_datalen
//    is not touched.
//  - the flags are handed to the underlying call unchanged (macOS 10.14 ignores unknown ones).

long sys_recvmsg_x(int socket, struct bsd_msghdr_x* msgp, unsigned int cnt, int flags)
{
	unsigned int i;
	CANCELATION_POINT();
	if (cnt == 0)
		return -EINVAL;
	if (msgp == NULL)
		return -EFAULT;
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
		if (msgp[i].hdr.msg_flags & BSD_MSG_TRUNC)
			return i + 1;
	}
	return i;
}

long sys_sendmsg_x(int socket, struct bsd_msghdr_x* msgp, unsigned int cnt, int flags)
{
	unsigned int i;
	CANCELATION_POINT();
	if (cnt == 0)
		return -EINVAL;
	if (msgp == NULL)
		return -EFAULT;
	for (i = 0; i < cnt; i++)
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
