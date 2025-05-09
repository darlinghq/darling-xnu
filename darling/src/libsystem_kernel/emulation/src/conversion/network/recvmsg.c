#include <darling/emulation/conversion/network/recvmsg.h>

#include <darling/emulation/conversion/network/getsockopt.h>

int socket_level_bsd_to_linux(int level)
{
	switch (level)
	{
		case BSD_SOL_SOCKET:
			return LINUX_SOL_SOCKET;
		default:
			return level;
	}
}

int socket_level_linux_to_bsd(int level)
{
	switch (level)
	{
		case LINUX_SOL_SOCKET:
			return BSD_SOL_SOCKET;
		default:
			return level;
	}
}
