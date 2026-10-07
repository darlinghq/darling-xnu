#include <darling/emulation/xnu_syscall/bsd/helper/ioctl/socket.h>

#include <sys/errno.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/sockio.h>
#include <netinet6/in6_var.h>
#include <darling/emulation/conversion/network/ifreq.h>

int handle_socket(int fd, unsigned int cmd, void* arg, int* retval)
{
	switch (cmd)
	{
        case SIOCGIFFLAGS:
        case SIOCGIFMTU: {
            if (!arg) {
                *retval = -EFAULT;
                return IOCTL_HANDLED;
            }
            struct ifreq* request = arg;
            struct linux_ifreq host_request = {0};
            __builtin_memcpy(host_request.lifr_name, request->ifr_name, LINUX_IFNAMSIZ);
            *retval = __real_ioctl(fd, cmd == SIOCGIFFLAGS ? LINUX_SIOCGIFFLAGS : LINUX_SIOCGIFMTU, &host_request);
            if (*retval >= 0) {
                if (cmd == SIOCGIFFLAGS)
                    request->ifr_flags = if_flags_linux_to_bsd(host_request.lifr_flags);
                else
                    request->ifr_mtu = host_request.lifr_mtu;
            }
            return IOCTL_HANDLED;
        }
        // Mutating network configuration remains unsupported.
		case SIOCSIFFLAGS:
		case SIOCAIFADDR: // set IPv4 address
		case SIOCAIFADDR_IN6: // set IPv6 address
			*retval = -ENOTSUP;
			return IOCTL_HANDLED;
	}
	return IOCTL_PASS;
}
