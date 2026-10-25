#include <darling/emulation/xnu_syscall/bsd/helper/misc/sysctl_net.h>

#include <sys/errno.h>
#include <net/route.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <darling/emulation/conversion/network/ifreq.h>
#include <netinet/in.h>
#include <net/if_var.h>
#include <stdint.h>
#include <limits.h>
#include <darling/emulation/common/simple.h>
#include <darling/emulation/common/string.h>
#include <darling/emulation/conversion/fcntl/open.h>
#include <darling/emulation/xnu_syscall/bsd/impl/fcntl/open.h>
#include <darling/emulation/xnu_syscall/bsd/helper/misc/readline.h>

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
static int do_iflist(void* old, unsigned long* oldlen, int modern, unsigned int index);
extern void* malloc(size_t size);
extern void free(void* ptr);

#define IFC_MAX_BYTES (1024 * 1024)
#define LINUX_SIOCGIFINDEX 0x8933

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
	_NET_RT_DUMP = 1,
	_NET_RT_IFLIST = 3,
	_NET_RT_IFLIST2 = 6,
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

static int number(const char** input, unsigned base, uint64_t* result) {
    const char* p = *input;
    while (*p == ' ' || *p == '\t') p++;
    uint64_t value = 0;
    unsigned count = 0;
    for (;;) {
        unsigned digit = *p >= '0' && *p <= '9' ? *p - '0' :
            *p >= 'a' && *p <= 'f' ? *p - 'a' + 10 :
            *p >= 'A' && *p <= 'F' ? *p - 'A' + 10 : base;
        if (digit >= base) break;
        if (value > (UINT64_MAX - digit) / base) return -EOVERFLOW;
        value = value * base + digit; p++; count++;
    }
    if (!count || (*p && *p != ' ' && *p != '\t')) return -EIO;
    *input = p; *result = value; return 0;
}

static int interface_statistics(const char* name, struct if_data64* stats) {
    int fd = sys_open_nocancel("/proc/net/dev", BSD_O_RDONLY, 0);
    if (fd < 0) return fd;
    struct rdline_buffer buffer;
    _readline_init(&buffer);
    const char* line;
    int status = -ENOENT;
    size_t length = strnlen(name, LINUX_IFNAMSIZ);
    while ((line = _readline(fd, &buffer)) != NULL) {
        while (*line == ' ' || *line == '\t') line++;
        if (strncmp(line, name, length) || line[length] != ':') continue;
        line += length + 1;
        uint64_t values[16];
        status = 0;
        for (int i = 0; i < 16; i++) {
            status = number(&line, 10, &values[i]);
            if (status) break;
        }
        if (status) break;
        stats->ifi_ibytes = values[0]; stats->ifi_ipackets = values[1];
        stats->ifi_ierrors = values[2]; stats->ifi_iqdrops = values[3];
        stats->ifi_imcasts = values[7]; stats->ifi_obytes = values[8];
        stats->ifi_opackets = values[9]; stats->ifi_oerrors = values[10];
        stats->ifi_collisions = values[13];
        break;
    }
    close_internal(fd);
    return status;
}

static int route_name(const char** input, char* name) {
    const char* p = *input;
    while (*p == ' ' || *p == '\t') p++;
    unsigned length = 0;
    while (*p && *p != ' ' && *p != '\t') {
        if (length >= LINUX_IFNAMSIZ - 1) return -EOVERFLOW;
        name[length++] = *p++;
    }
    if (!length) return -EIO;
    name[length] = 0; *input = p; return 0;
}

static int ipv6_bytes(const char** input, unsigned char* bytes) {
    const char* p = *input;
    while (*p == ' ' || *p == '\t') p++;
    for (unsigned i = 0; i < 16; i++) {
        unsigned value = 0;
        for (unsigned j = 0; j < 2; j++) {
            unsigned digit = *p >= '0' && *p <= '9' ? *p - '0' :
                *p >= 'a' && *p <= 'f' ? *p - 'a' + 10 :
                *p >= 'A' && *p <= 'F' ? *p - 'A' + 10 : 16;
            if (digit == 16) return -EIO;
            value = value * 16 + digit; p++;
        }
        bytes[i] = value;
    }
    if (*p && *p != ' ' && *p != '\t') return -EIO;
    *input = p; return 0;
}

static int append_route(int socket, const char* name, int family, uint64_t flags,
    const unsigned char* destination, const unsigned char* gateway, const unsigned char* mask,
    void* output, unsigned long capacity, unsigned long* total) {
    struct linux_ifreq request = {0};
    strlcpy(request.lifr_name, name, LINUX_IFNAMSIZ);
    int status = __real_ioctl(socket, LINUX_SIOCGIFINDEX, &request);
    if (status < 0) return status;
    if (request.lifr_ifindex <= 0 || request.lifr_ifindex > UINT16_MAX) return -EOVERFLOW;
    size_t address_length = family == AF_INET ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6);
    size_t length = sizeof(struct rt_msghdr) + 3 * address_length;
    if (*total > ULONG_MAX - length) return -EOVERFLOW;
    if (output && capacity >= *total + length) {
        struct rt_msghdr* header = (void*)((char*)output + *total);
        memset(header, 0, length);
        header->rtm_msglen = length; header->rtm_version = RTM_VERSION;
        header->rtm_type = RTM_GET; header->rtm_index = request.lifr_ifindex;
        header->rtm_addrs = RTA_DST | RTA_GATEWAY | RTA_NETMASK;
        if (flags & 1) header->rtm_flags |= RTF_UP;
        if (flags & 2) header->rtm_flags |= RTF_GATEWAY;
        if (flags & 4) header->rtm_flags |= RTF_HOST;
        if (flags & 0x10) header->rtm_flags |= RTF_DYNAMIC;
        if (flags & 0x20) header->rtm_flags |= RTF_MODIFIED;
        if (flags & 0x200) header->rtm_flags |= RTF_REJECT;
        const unsigned char* values[] = {destination, gateway, mask};
        for (unsigned i = 0; i < 3; i++) {
            char* address = (char*)(header + 1) + i * address_length;
            if (family == AF_INET) {
                struct sockaddr_in* ipv4 = (void*)address;
                ipv4->sin_len = address_length; ipv4->sin_family = AF_INET;
                memcpy(&ipv4->sin_addr, values[i], 4);
            } else {
                struct sockaddr_in6* ipv6 = (void*)address;
                ipv6->sin6_len = address_length; ipv6->sin6_family = AF_INET6;
                memcpy(&ipv6->sin6_addr, values[i], 16);
                if (values[i][0] == 0xfe && (values[i][1] & 0xc0) == 0x80)
                    ipv6->sin6_scope_id = request.lifr_ifindex;
            }
        }
    }
    *total += length;
    return 0;
}

static int route_file(int socket, int family, void* output, unsigned long capacity, unsigned long* total) {
    int fd = sys_open_nocancel(family == AF_INET ? "/proc/net/route" : "/proc/net/ipv6_route", BSD_O_RDONLY, 0);
    if (fd < 0) return family == AF_INET6 && fd == -ENOENT ? 0 : fd;
    struct rdline_buffer buffer; _readline_init(&buffer);
    const char* line;
    int status = 0;
    while ((line = _readline(fd, &buffer)) != NULL) {
        char name[LINUX_IFNAMSIZ];
        unsigned char destination[16] = {0}, gateway[16] = {0}, mask[16] = {0};
        uint64_t flags;
        if (family == AF_INET) {
            if (strncmp(line, "Iface", 5) == 0) continue;
            if ((status = route_name(&line, name)) < 0) break;
            uint64_t fields[10];
            for (unsigned i = 0; i < 10; i++) {
                status = number(&line, i == 0 || i == 1 || i == 2 || i == 6 ? 16 : 10, &fields[i]);
                if (status) break;
            }
            if (status) break;
            if (fields[0] > UINT32_MAX || fields[1] > UINT32_MAX || fields[6] > UINT32_MAX) {status = -EOVERFLOW; break;}
            uint32_t address = fields[0]; memcpy(destination, &address, 4);
            address = fields[1]; memcpy(gateway, &address, 4);
            address = fields[6]; memcpy(mask, &address, 4); flags = fields[2];
        } else {
            unsigned char source[16];
            uint64_t prefix, source_prefix, metric, refs, use;
            if ((status = ipv6_bytes(&line, destination)) < 0 || (status = number(&line, 16, &prefix)) < 0 ||
                (status = ipv6_bytes(&line, source)) < 0 || (status = number(&line, 16, &source_prefix)) < 0 ||
                (status = ipv6_bytes(&line, gateway)) < 0 || (status = number(&line, 16, &metric)) < 0 ||
                (status = number(&line, 16, &refs)) < 0 || (status = number(&line, 16, &use)) < 0 ||
                (status = number(&line, 16, &flags)) < 0 || (status = route_name(&line, name)) < 0) break;
            if (prefix > 128 || source_prefix > 128) {status = -EIO; break;}
            // Source-specific routes cannot be represented by this legacy ABI.
            if (source_prefix) continue;
            for (unsigned bit = 0; bit < prefix; bit++) mask[bit / 8] |= 0x80 >> (bit % 8);
        }
        status = append_route(socket, name, family, flags, destination, gateway, mask, output, capacity, total);
        if (status) break;
    }
    close_internal(fd); return status;
}

static int do_routes(int family, void* output, unsigned long* length) {
    if (family != 0 && family != AF_INET && family != AF_INET6) return -EAFNOSUPPORT;
    int socket = sys_socket(AF_INET, SOCK_DGRAM, 0);
    if (socket < 0) return socket;
    unsigned long capacity = *length, total = 0;
    int status = 0;
    if (family == 0 || family == AF_INET) status = route_file(socket, AF_INET, output, capacity, &total);
    if (!status && (family == 0 || family == AF_INET6)) status = route_file(socket, AF_INET6, output, capacity, &total);
    close_internal(socket); *length = total;
    if (!status && output && capacity < total) status = -ENOMEM;
    return status;
}

static int do_iflist(void* old, unsigned long* oldlen, int modern, unsigned int wanted_index) {
	// we only do IPv4 addresses for now
	int tmp_sock;
	char* buf = NULL;
	unsigned long capacity = *oldlen;
	struct linux_ifconf lifc = {
		.lifc_len = 0,
		.lifc_buf = NULL,
	};
	int status = 0;
	char* old_ptr = old;
	unsigned long total_len = 0;

	if ((status = tmp_sock = sys_socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		goto out;

    // Linux first reports the required buffer size; retry if interfaces grow.
    if ((status = __real_ioctl(tmp_sock, LINUX_SIOCGIFCONF, &lifc)) < 0) goto out;
    if (lifc.lifc_len < 0 || lifc.lifc_len > IFC_MAX_BYTES - 2 * (int)sizeof(struct linux_ifreq)) { status = -EOVERFLOW; goto out; }
    int bytes = lifc.lifc_len + 2 * sizeof(struct linux_ifreq);
    for (;;) {
        if (bytes <= 0 || bytes > IFC_MAX_BYTES) { status = -EOVERFLOW; goto out; }
        buf = malloc(bytes);
        if (!buf) { status = -ENOMEM; goto out; }
        lifc.lifc_buf = buf; lifc.lifc_len = bytes;
        if ((status = __real_ioctl(tmp_sock, LINUX_SIOCGIFCONF, &lifc)) < 0) goto out;
        if (lifc.lifc_len <= bytes - (int)sizeof(struct linux_ifreq)) break;
        free(buf); buf = NULL; bytes *= 2;
    }
    if (lifc.lifc_len < 0 || lifc.lifc_len % sizeof(struct linux_ifreq)) { status = -EIO; goto out; }

	for (int i = 0; i < lifc.lifc_len; i += sizeof(struct linux_ifreq)) {
		struct linux_ifreq* curr = &lifc.lifc_req[i / sizeof(struct linux_ifreq)];
        struct linux_ifreq index_request = {0};
        memcpy(index_request.lifr_name, curr->lifr_name, LINUX_IFNAMSIZ);
        if ((status = __real_ioctl(tmp_sock, LINUX_SIOCGIFINDEX, &index_request)) < 0) goto out;
        unsigned int iface_idx = index_request.lifr_ifindex;
        if (!iface_idx || iface_idx > UINT16_MAX) { status = -EOVERFLOW; goto out; }
        if (wanted_index && iface_idx != wanted_index) continue;
		size_t name_len = strnlen(curr->lifr_name, sizeof(curr->lifr_name));
		size_t additional_sockaddr_len = name_len + MAC_LENGTH;
		additional_sockaddr_len = (additional_sockaddr_len <= sizeof(((struct sockaddr_dl*)NULL)->sdl_data)) ? 0 : (additional_sockaddr_len - sizeof(((struct sockaddr_dl*)NULL)->sdl_data));
        size_t header_length = modern ? sizeof(struct if_msghdr2) : sizeof(struct if_msghdr);
		size_t ifm_length = header_length + round_up_sizeof_u32(sizeof(struct sockaddr_dl) + additional_sockaddr_len);
		size_t ifam_length = sizeof(struct ifa_msghdr) + round_up_sizeof_u32(sizeof(struct sockaddr)) * 3;


		if (old && capacity >= total_len + ifm_length) {
			struct if_msghdr* ifm = (void*)old_ptr;
			struct sockaddr_dl* dl = (void*)(old_ptr + header_length);
			struct linux_ifreq tmp;

			memset(old_ptr, 0, ifm_length);

			ifm->ifm_msglen = ifm_length;
			ifm->ifm_index = iface_idx;
			ifm->ifm_type = modern ? RTM_IFINFO2 : RTM_IFINFO;
			ifm->ifm_version = RTM_VERSION;
			ifm->ifm_addrs = RTA_IFP;

			dl->sdl_len = sizeof(struct sockaddr_dl) + additional_sockaddr_len;
			dl->sdl_family = AF_LINK;
			dl->sdl_index = iface_idx;
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

            struct linux_ifreq mtu_request = {0};
            memcpy(mtu_request.lifr_name, curr->lifr_name, LINUX_IFNAMSIZ);
            if ((status = __real_ioctl(tmp_sock, LINUX_SIOCGIFMTU, &mtu_request)) < 0) goto out;
            if (modern) {
                struct if_msghdr2* info = (void*)old_ptr;
                if ((status = interface_statistics(curr->lifr_name, &info->ifm_data)) < 0) goto out;
                info->ifm_data.ifi_type = dl->sdl_type;
                info->ifm_data.ifi_addrlen = dl->sdl_alen;
                info->ifm_data.ifi_mtu = mtu_request.lifr_mtu;
            } else {
                ifm->ifm_data.ifi_type = dl->sdl_type;
                ifm->ifm_data.ifi_addrlen = dl->sdl_alen;
                ifm->ifm_data.ifi_mtu = mtu_request.lifr_mtu;
            }

			old_ptr += ifm_length;
		}
		total_len += ifm_length;

		if (old && capacity >= total_len + ifam_length) {
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

	// A size probe succeeds; a short output buffer reports its required length.
	status = old && capacity < total_len ? -ENOMEM : 0;

out:
    free(buf);
	if (tmp_sock >= 0) {
		// ignore errors while closing
		close_internal(tmp_sock);
	}
	*oldlen = total_len;
	return status;
};

static sysctl_handler(handle_route) {
	int status = 0;

	if (nlen != 6 || !oldlen) {
		status = -EINVAL;
		goto out;
	}

	if (_new || newlen) return -EPERM;

	// name[4] is the desired operation
	switch (name[4]) {
        case _NET_RT_DUMP:
            status = do_routes(name[3], old, oldlen);
            break;
		case _NET_RT_IFLIST:
        case _NET_RT_IFLIST2:
			status = do_iflist(old, oldlen, name[4] == _NET_RT_IFLIST2, name[5]);
			break;
		default:
			status = -EINVAL;
			goto out;
	}

out:
	return status;
}
