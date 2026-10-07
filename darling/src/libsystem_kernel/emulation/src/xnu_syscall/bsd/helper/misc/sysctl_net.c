#include <darling/emulation/xnu_syscall/bsd/helper/misc/sysctl_net.h>

#include <sys/errno.h>
#include <net/route.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <darling/emulation/conversion/network/ifreq.h>
#include <netinet/in.h>

#include <darling/emulation/xnu_syscall/bsd/impl/network/socket.h>
#include <darling/emulation/xnu_syscall/bsd/helper/network/duct.h>
#include <darling/emulation/xnu_syscall/bsd/impl/ioctl/ioctl.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/close.h>

extern void* memset(void* destination, int value, size_t length);
extern void* memcpy(void* destination, const void* source, size_t n);
extern size_t strnlen(const char* string, size_t max_length);
extern char* strncpy(char* destination, const char* source, size_t length);
extern size_t strlcpy(char* destination, const char* source, size_t size);

static sysctl_handler(handle_route);
static int do_iflist(void* old, unsigned long* oldlen);

// arbitrary number; only exists to avoid blowing up the stack
#define IFC_MAX_INTERFACES 5

struct linux_ifconf {
	int lifc_len;
	union {
		char* lifc_buf;
		struct linux_ifreq* lifc_req;
	};
};

enum {
	_NET_ROUTE = 17,
};

enum {
	_NET_RT_IFLIST = 3,
};

const struct known_sysctl sysctls_net[] = {
	// technically a CTLTYPE_NODE in XNU, but for us that means that it has children and isn't a variable in and of itself.
	// the actual value doesn't really matter as long as it's not CTLTYPE_NODE
	{ .oid = _NET_ROUTE, .type = CTLTYPE_OPAQUE, .exttype = "", .name = "route", .handler = handle_route },
	{ .oid = -1 },
};

static unsigned long round_up_sizeof_u32(unsigned long number) {
	return (number + (sizeof(uint32_t) - 1)) & ~(sizeof(uint32_t) - 1);
}

static int do_iflist(void* old, unsigned long* oldlen) {
	// we only do IPv4 addresses for now
	int tmp_sock;
	char buf[IFC_MAX_INTERFACES * sizeof(struct linux_ifreq)];
	struct linux_ifconf lifc = {
		.lifc_len = sizeof(buf),
		.lifc_buf = buf,
	};
	int status = 0;
	char* old_ptr = old;
	unsigned long total_len = 0;

	if ((status = tmp_sock = sys_socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		goto out;

	if ((status = __real_ioctl(tmp_sock, LINUX_SIOCGIFCONF, &lifc)) < 0)
		goto out;

	for (int i = 0; i < lifc.lifc_len; i += sizeof(struct linux_ifreq)) {
		struct linux_ifreq* curr = &lifc.lifc_req[i / sizeof(struct linux_ifreq)];
		size_t name_len = strnlen(curr->lifr_name, sizeof(curr->lifr_name));
		size_t additional_sockaddr_len = name_len + MAC_LENGTH;
		additional_sockaddr_len = (additional_sockaddr_len <= sizeof(((struct sockaddr_dl*)NULL)->sdl_data)) ? 0 : (additional_sockaddr_len - sizeof(((struct sockaddr_dl*)NULL)->sdl_data));
		size_t ifm_length = sizeof(struct if_msghdr) + round_up_sizeof_u32(sizeof(struct sockaddr_dl) + additional_sockaddr_len);
		size_t ifam_length = sizeof(struct ifa_msghdr) + round_up_sizeof_u32(sizeof(struct sockaddr)) * 3;
		// technically not correct; SIOCGIFCONF could return the same interfaces in a different order on each call
		// but this is enough for our purposes
		size_t iface_idx = (i / sizeof(struct linux_ifreq)) + 1;

		if (old && *oldlen >= total_len + ifm_length) {
			struct if_msghdr* ifm = (void*)old_ptr;
			struct sockaddr_dl* dl = (void*)(old_ptr + sizeof(struct if_msghdr));
			struct linux_ifreq tmp;

			memset(old_ptr, 0, ifm_length);

			ifm->ifm_msglen = ifm_length;
			ifm->ifm_index = iface_idx;
			ifm->ifm_type = RTM_IFINFO;
			ifm->ifm_version = RTM_VERSION;
			ifm->ifm_addrs = RTA_IFP;

			dl->sdl_len = sizeof(struct sockaddr_dl) + additional_sockaddr_len;
			dl->sdl_family = AF_LINK;
			dl->sdl_index = ifm->ifm_index;
			dl->sdl_type = IFT_OTHER;
			dl->sdl_nlen = name_len;
			dl->sdl_alen = MAC_LENGTH;
			dl->sdl_slen = 0; // ???

			memcpy(dl->sdl_data, curr->lifr_name, name_len);

            // Translate Linux interface flags rather than leaving every interface down.
            memcpy(tmp.lifr_name, curr->lifr_name, sizeof(curr->lifr_name));
            if ((status = __real_ioctl(tmp_sock, LINUX_SIOCGIFFLAGS, &tmp)) < 0)
                goto out;
            ifm->ifm_flags = if_flags_linux_to_bsd(tmp.lifr_flags);

			// grab the MAC
			memcpy(tmp.lifr_name, curr->lifr_name, sizeof(curr->lifr_name));
			if ((status = __real_ioctl(tmp_sock, LINUX_SIOCGIFHWADDR, &tmp)) < 0)
				goto out;
            // Linux returns an ARPHRD_* type with the link-layer address.
            if (tmp.lifr_hwaddr.sa_family == 1) // ARPHRD_ETHER
                dl->sdl_type = IFT_ETHER;
            else if (tmp.lifr_hwaddr.sa_family == 772) // ARPHRD_LOOPBACK
                dl->sdl_type = IFT_LOOP;
            else if (tmp.lifr_hwaddr.sa_family == 512) // ARPHRD_PPP
                dl->sdl_type = IFT_PPP;
			memcpy(LLADDR(dl), tmp.lifr_hwaddr.sa_data, MAC_LENGTH);

			old_ptr += ifm_length;
		}
		total_len += ifm_length;

		if (old && *oldlen >= total_len + ifam_length) {
			struct ifa_msghdr* ifam = (void*)old_ptr;
			struct sockaddr* netmask = (void*)(old_ptr + sizeof(struct ifa_msghdr));
			struct sockaddr* ip_addr = (void*)(old_ptr + sizeof(struct ifa_msghdr) + round_up_sizeof_u32(sizeof(struct sockaddr)));
			struct sockaddr* broad_addr = (void*)(old_ptr + sizeof(struct ifa_msghdr) + round_up_sizeof_u32(sizeof(struct sockaddr)) * 2);
			struct linux_ifreq tmp;

			memset(old_ptr, 0, ifam_length);

			ifam->ifam_msglen = ifam_length;
			ifam->ifam_index = iface_idx;
			ifam->ifam_type = RTM_NEWADDR;
			ifam->ifam_version = RTM_VERSION;
			ifam->ifam_addrs = RTA_NETMASK | RTA_IFA | RTA_BRD;

			sockaddr_fixup_from_linux((struct sockaddr_fixup*)ip_addr, &curr->lifr_addr, sizeof(struct linux_sockaddr));

			memcpy(tmp.lifr_name, curr->lifr_name, sizeof(curr->lifr_name));

			// get the netmask
			if ((status = __real_ioctl(tmp_sock, LINUX_SIOCGIFNETMASK, &tmp)) < 0)
				goto out;
			sockaddr_fixup_from_linux((struct sockaddr_fixup*)netmask, &tmp.lifr_netmask, sizeof(struct linux_sockaddr));

			// get the broadcast address
			if ((status = __real_ioctl(tmp_sock, LINUX_SIOCGIFBRDADDR, &tmp)) < 0)
				goto out;
			sockaddr_fixup_from_linux((struct sockaddr_fixup*)broad_addr, &tmp.lifr_broadaddr, sizeof(struct linux_sockaddr));

			old_ptr += ifam_length;
		}
		total_len += ifam_length;
	}

	// we succeeded if we got here
	status = 0;

out:
	if (tmp_sock >= 0) {
		// ignore errors while closing
		close_internal(tmp_sock);
	}
	*oldlen = total_len;
	return status;
};

static sysctl_handler(handle_route) {
	int status = 0;

	if (nlen != 6) {
		status = -EINVAL;
		goto out;
	}

	// name[4] is the desired operation
	switch (name[4]) {
		case _NET_RT_IFLIST:
			status = do_iflist(old, oldlen);
			break;
		default:
			status = -EINVAL;
			goto out;
	}

out:
	return status;
}
