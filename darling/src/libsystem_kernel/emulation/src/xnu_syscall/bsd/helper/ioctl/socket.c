#include <darling/emulation/xnu_syscall/bsd/helper/ioctl/socket.h>

#include <sys/errno.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <net/if_media.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <stddef.h>
#include <netinet/in.h>
#include <sys/sockio.h>
#include <netinet6/in6_var.h>
#include <darling/emulation/conversion/network/ifreq.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/conversion/duct_errno.h>

#define LINUX_ARPHRD_ETHER 1
#define LINUX_ARPHRD_LOOPBACK 772
#define LINUX_AF_INET 2

// Linux returns one record per IPv4 address; this bounds the stack buffer for them.
#define IFCONF_MAX_RECORDS 16

extern void* memset(void* destination, int value, size_t length);
extern void* memcpy(void* destination, const void* source, size_t n);
extern size_t strnlen(const char* string, size_t max_length);

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
        case SIOCGIFCONF: {
            // Darwin returns, per interface, a link-level record and then one record per
            // address. Linux knows the request under another number and only returns the
            // IPv4 addresses, so the link records are built here, the way NET_RT_IFLIST
            // builds them. A caller that sees this request fail may not close its socket.
            struct ifconf* ifc = (struct ifconf*) arg;
            char lbuf[IFCONF_MAX_RECORDS * sizeof(struct linux_ifreq)];
            struct linux_ifconf lifc = {
                .lifc_len = sizeof(lbuf),
                .lifc_buf = lbuf,
            };
            int status;
            int space;
            int written = 0;

            if (ifc == NULL) {
                *retval = -EFAULT;
                return IOCTL_HANDLED;
            }

            status = __real_ioctl(fd, LINUX_SIOCGIFCONF, &lifc);
            if (status < 0) {
                *retval = status;
                return IOCTL_HANDLED;
            }

            space = (ifc->ifc_buf != NULL && ifc->ifc_len > 0) ? ifc->ifc_len : 0;

            for (int i = 0; i + (int) sizeof(struct linux_ifreq) <= lifc.lifc_len; i += sizeof(struct linux_ifreq)) {
                struct linux_ifreq* curr = &lifc.lifc_req[i / sizeof(struct linux_ifreq)];
                struct linux_ifreq hw;
                size_t name_len = strnlen(curr->lifr_name, LINUX_IFNAMSIZ - 1);
                size_t dl_len = offsetof(struct sockaddr_dl, sdl_data) + name_len + MAC_LENGTH;
                size_t record;
                struct ifreq* ifr;
                struct sockaddr_dl* dl;

                if (dl_len < sizeof(struct sockaddr_dl))
                    dl_len = sizeof(struct sockaddr_dl);

                // A record is an ifreq, or longer when its address does not fit in one.
                record = IFNAMSIZ + dl_len;
                if (record < sizeof(struct ifreq))
                    record = sizeof(struct ifreq);
                if ((size_t) (space - written) < record)
                    break;

                ifr = (struct ifreq*) (ifc->ifc_buf + written);
                memset(ifr, 0, record);
                memcpy(ifr->ifr_name, curr->lifr_name, name_len);

                dl = (struct sockaddr_dl*) &ifr->ifr_addr;
                dl->sdl_len = dl_len;
                dl->sdl_family = AF_LINK;
                dl->sdl_index = (i / sizeof(struct linux_ifreq)) + 1;
                dl->sdl_nlen = name_len;
                memcpy(dl->sdl_data, curr->lifr_name, name_len);

                memset(&hw, 0, sizeof(hw));
                memcpy(hw.lifr_name, curr->lifr_name, LINUX_IFNAMSIZ);
                if (__real_ioctl(fd, LINUX_SIOCGIFHWADDR, &hw) == 0 && hw.lifr_addr.sa_family == LINUX_ARPHRD_ETHER) {
                    dl->sdl_type = IFT_ETHER;
                    dl->sdl_alen = MAC_LENGTH;
                    memcpy(LLADDR(dl), hw.lifr_addr.sa_data, MAC_LENGTH);
                } else {
                    dl->sdl_type = (hw.lifr_addr.sa_family == LINUX_ARPHRD_LOOPBACK) ? IFT_LOOP : IFT_OTHER;
                }
                written += record;

                if (curr->lifr_addr.sa_family != LINUX_AF_INET)
                    continue;
                if ((size_t) (space - written) < sizeof(struct ifreq))
                    break;

                ifr = (struct ifreq*) (ifc->ifc_buf + written);
                memset(ifr, 0, sizeof(struct ifreq));
                memcpy(ifr->ifr_name, curr->lifr_name, name_len);
                ifr->ifr_addr.sa_len = sizeof(struct sockaddr_in);
                ifr->ifr_addr.sa_family = AF_INET;
                memcpy(ifr->ifr_addr.sa_data, curr->lifr_addr.sa_data, sizeof(curr->lifr_addr.sa_data));
                written += sizeof(struct ifreq);
            }

            ifc->ifc_len = written;
            *retval = 0;
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
