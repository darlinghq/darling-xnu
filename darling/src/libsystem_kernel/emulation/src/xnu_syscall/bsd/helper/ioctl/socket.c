#include <darling/emulation/xnu_syscall/bsd/helper/ioctl/socket.h>

#include <sys/errno.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <net/if_media.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <stddef.h>
#include <sys/mman.h>
#include <darling/emulation/xnu_syscall/bsd/impl/mman/mman.h>
#include <netinet/in.h>
#include <sys/sockio.h>
#include <netinet6/in6_var.h>
#include <darling/emulation/conversion/network/ifreq.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/conversion/duct_errno.h>

#define LINUX_ARPHRD_ETHER 1
#define LINUX_ARPHRD_LOOPBACK 772
#define LINUX_AF_INET 2

// Upper bound on the size of the address list read from Linux.
#define IFCONF_MAX_BYTES (1024 * 1024)

extern void* memset(void* destination, int value, size_t length);
extern void* memcpy(void* destination, const void* source, size_t n);
extern size_t strnlen(const char* string, size_t max_length);

// Length of an interface name without a Linux alias label ("name:label").
static size_t ifconf_base_name_length(const char* name)
{
	size_t length = 0;
	while (length < LINUX_IFNAMSIZ - 1 && name[length] != '\0' && name[length] != ':')
		length++;
	return length;
}

// Answers SIOCGIFCONF from Linux's address list. Darwin lists, for each interface, a
// link-level record followed by one record per address; Linux lists IPv4 addresses only.
// Records have variable length (name plus sockaddr), only whole records that fit in
// ifc_len are written, and ifc_len is set to the number of bytes written. The call
// succeeds when the buffer is too short for everything.
static int ifconf_from_linux(int fd, struct ifconf* ifc)
{
	char* list = NULL;
	int list_size = 0;
	int status;
	int space;
	int written = 0;
	struct linux_ifconf lifc = { .lifc_len = 0, .lifc_buf = NULL };
	const char* previous = NULL;
	size_t previous_length = 0;

	if (ifc == NULL)
		return -EFAULT;

	space = ifc->ifc_len > 0 ? ifc->ifc_len : 0;
	if (ifc->ifc_buf == NULL)
	{
		if (space > 0)
			return -EFAULT;
		ifc->ifc_len = 0;
		return 0;
	}

	// Linux reports the size it needs when no buffer is given; retry if the list grows.
	status = __real_ioctl(fd, LINUX_SIOCGIFCONF, &lifc);
	if (status < 0)
		return status;
	list_size = lifc.lifc_len;
	for (;;)
	{
		if (list_size < 0 || list_size > IFCONF_MAX_BYTES - 2 * (int) sizeof(struct linux_ifreq))
			return -EOVERFLOW;
		list_size += 2 * sizeof(struct linux_ifreq);
		// Mapped rather than allocated: libsystem_kernel sits below malloc.
		list = sys_mmap(NULL, list_size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
		if ((unsigned long) list > -4096ul)
			return -ENOMEM;
		lifc.lifc_buf = list;
		lifc.lifc_len = list_size;
		status = __real_ioctl(fd, LINUX_SIOCGIFCONF, &lifc);
		if (status < 0 || lifc.lifc_len <= list_size - (int) sizeof(struct linux_ifreq))
			break;
		sys_munmap(list, list_size);
		list = NULL;
		list_size *= 2;
	}
	if (status < 0)
	{
		sys_munmap(list, list_size);
		return status;
	}

	for (int i = 0; i + (int) sizeof(struct linux_ifreq) <= lifc.lifc_len; i += sizeof(struct linux_ifreq))
	{
		struct linux_ifreq* curr = &lifc.lifc_req[i / sizeof(struct linux_ifreq)];
		size_t name_len = ifconf_base_name_length(curr->lifr_name);
		struct ifreq* ifr;

		if (curr->lifr_addr.sa_family != LINUX_AF_INET)
			continue;

		// The link-level record comes once per interface, before its first address.
		if (previous == NULL || previous_length != name_len || __builtin_memcmp(previous, curr->lifr_name, name_len) != 0)
		{
			struct linux_ifreq hw = {0};
			struct linux_ifreq index = {0};
			struct sockaddr_dl* dl;
			size_t dl_len = offsetof(struct sockaddr_dl, sdl_data) + name_len;
			size_t record;
			int ethernet;

			memcpy(hw.lifr_name, curr->lifr_name, name_len);
			ethernet = __real_ioctl(fd, LINUX_SIOCGIFHWADDR, &hw) == 0 && hw.lifr_hwaddr.sa_family == LINUX_ARPHRD_ETHER;
			if (ethernet)
				dl_len += MAC_LENGTH;
			if (dl_len < sizeof(struct sockaddr_dl))
				dl_len = sizeof(struct sockaddr_dl);

			// A record is an ifreq, or longer when its address does not fit in one.
			record = IFNAMSIZ + dl_len;
			if (record < sizeof(struct ifreq))
				record = sizeof(struct ifreq);
			if ((size_t) (space - written) < record)
				break;

			memcpy(index.lifr_name, curr->lifr_name, name_len);
			if (__real_ioctl(fd, LINUX_SIOCGIFINDEX, &index) < 0)
				index.lifr_ifindex = 0;

			ifr = (struct ifreq*) (ifc->ifc_buf + written);
			memset(ifr, 0, record);
			memcpy(ifr->ifr_name, curr->lifr_name, name_len);

			dl = (struct sockaddr_dl*) &ifr->ifr_addr;
			dl->sdl_len = dl_len;
			dl->sdl_family = AF_LINK;
			dl->sdl_index = index.lifr_ifindex;
			dl->sdl_nlen = name_len;
			memcpy(dl->sdl_data, curr->lifr_name, name_len);
			if (ethernet)
			{
				dl->sdl_type = IFT_ETHER;
				dl->sdl_alen = MAC_LENGTH;
				memcpy(LLADDR(dl), hw.lifr_hwaddr.sa_data, MAC_LENGTH);
			}
			else
			{
				dl->sdl_type = (hw.lifr_hwaddr.sa_family == LINUX_ARPHRD_LOOPBACK) ? IFT_LOOP : IFT_OTHER;
			}
			written += record;
			previous = curr->lifr_name;
			previous_length = name_len;
		}

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

	sys_munmap(list, list_size);
	ifc->ifc_len = written;
	return 0;
}

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
        case SIOCGIFCONF:
            *retval = ifconf_from_linux(fd, arg);
            return IOCTL_HANDLED;
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
