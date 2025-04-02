#include <emulation/xnu_syscall/bsd/impl/ioctl/ioctl.h>

#include <emulation/conversion/methods/errno.h>
#include <emulation/conversion/variables/bsd/sys/sockio.h>
#include <emulation/conversion/variables/bsd/netinet6/in6_var.h>

int handle_socket(int fd, unsigned int cmd, void* arg, int* retval)
{
	switch (cmd)
	{
		// TODO: This has to be implemented for container with separate networking
		case BSD_SIOCGIFFLAGS:
		case BSD_SIOCSIFFLAGS:
		case BSD_SIOCAIFADDR: // set IPv4 address
		case BSD_SIOCAIFADDR_IN6: // set IPv6 address
			*retval = -BSD_ENOTSUP;
			return IOCTL_HANDLED;
	}
	return IOCTL_PASS;
}


