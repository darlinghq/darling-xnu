#include <darling/emulation/xnu_syscall/bsd/helper/ioctl/socket.h>

#include <sys/errno.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <net/if_media.h>
#include <netinet/in.h>
#include <sys/sockio.h>
#include <netinet6/in6_var.h>
#include <darling/emulation/conversion/network/ifreq.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/conversion/duct_errno.h>

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
        case SIOCGIFMEDIA:
        case SIOCGIFXMEDIA: {
            if (!arg) {
                *retval = -EFAULT;
                return IOCTL_HANDLED;
            }
            struct ifmediareq* request = arg;
            struct linux_ifreq host_request = {0};
            __builtin_memcpy(host_request.lifr_name, request->ifm_name, LINUX_IFNAMSIZ);
            *retval = __real_ioctl(fd, LINUX_SIOCGIFFLAGS, &host_request);
            if (*retval < 0) {
                // XNU's ifioctl returns ENXIO for an unknown interface; Linux says ENODEV.
                *retval = *retval == -LINUX_ENODEV ? -ENXIO : errno_linux_to_bsd(*retval);
                return IOCTL_HANDLED;
            }
            unsigned int flags = if_flags_linux_to_bsd(host_request.lifr_flags);
            // XNU's lo0 does not implement media requests.
            if (flags & IFF_LOOPBACK) {
                *retval = -EOPNOTSUPP;
                return IOCTL_HANDLED;
            }
            // Linux has no media word; report a single autoselect medium,
            // 802.11 for Linux wireless names (wl*), Ethernet otherwise.
            int wireless = request->ifm_name[0] == 'w' && request->ifm_name[1] == 'l';
            int media = (wireless ? IFM_IEEE80211 : IFM_ETHER) | IFM_AUTO;
            request->ifm_current = media;
            request->ifm_active = media;
            request->ifm_mask = 0;
            request->ifm_status = IFM_AVALID;
            if ((flags & IFF_UP) && (flags & IFF_RUNNING))
                request->ifm_status |= IFM_ACTIVE;
            // As XNU drivers do: a NULL list asks for the count; otherwise fill up to ifm_count entries.
            if (request->ifm_ulist == NULL) {
                request->ifm_count = 1;
            } else if (request->ifm_count > 0) {
                request->ifm_ulist[0] = media;
                request->ifm_count = 1;
            }
            *retval = 0;
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
