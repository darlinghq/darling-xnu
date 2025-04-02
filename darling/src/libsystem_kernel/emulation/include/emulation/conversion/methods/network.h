#ifndef DARLING_EMULATION_CONVERSION_NETWORK_H
#define DARLING_EMULATION_CONVERSION_NETWORK_H

#include <emulation/conversion/variables/bsd/netinet/in.h>
#include <emulation/conversion/variables/bsd/netinet/tcp.h>
#include <emulation/conversion/variables/bsd/sys/socket.h>
#include <emulation/conversion/variables/bsd/sys/un.h>
#include <emulation/conversion/variables/linux/linux/in.h>
#include <emulation/conversion/variables/linux/linux/in6.h>
#include <emulation/conversion/variables/linux/linux/net.h>
#include <emulation/conversion/variables/linux/linux/tcp.h>
#include <emulation/conversion/variables/linux/sys/socket.h>

#define LINGER_TICKS_PER_SEC	100 // Is this the right number of ticks per sec?

int sfamily_linux_to_bsd(int family);
int sfamily_bsd_to_linux(int family);
int msgflags_bsd_to_linux(int flags);
int msgflags_linux_to_bsd(int flags);
int sockopt_bsd_to_linux(int* level, int* optname, void** optval, void* optbuf);

struct sockaddr_fixup
{
	union
	{
		unsigned short linux_family;
		struct
		{
			unsigned char bsd_length;
			unsigned char bsd_family;
		};
	};
	union
	{
		char sun_path[104];
	};
};

unsigned long sockaddr_fixup_size_from_bsd(const void* bsd_sockaddr, int bsd_sockaddr_len);
int sockaddr_fixup_from_bsd(struct sockaddr_fixup* out, const void* bsd_sockaddr, int bsd_sockaddr_len);
int sockaddr_fixup_from_linux(struct sockaddr_fixup* out, const void* linux_sockaddr, int linux_sockaddr_len);

#endif // DARLING_EMULATION_CONVERSION_NETWORK_H
