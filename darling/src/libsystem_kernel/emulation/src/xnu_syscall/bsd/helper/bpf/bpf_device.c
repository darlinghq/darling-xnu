/**
 * This file is part of Darling.
 *
 * Copyright (C) 2026 Lubos Dolezel
 *
 * Darling is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Darling is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Darling.  If not, see <http://www.gnu.org/licenses/>.
 */

// /dev/bpfN devices (bpf(4)) on top of Linux AF_PACKET sockets (packet(7)).
//
// Opening a device creates a raw packet socket that receives nothing until
// BIOCSETIF binds it to an interface. Everything XNU keeps in its bpf_d (buffer
// size, filter, flags, the packet that did not fit into the last read) lives
// in a struct bpf_desc here, shared by all descriptors dup'ed from the same
// open. read() turns the packets queued on the socket into the bpf(4) buffer
// format: records made of a struct bpf_hdr, the captured bytes, and padding
// to the next 32-bit boundary.
//
// Link types: Ethernet interfaces are DLT_EN10MB and Linux delivers the same
// frames, so filters run in the Linux kernel (SO_ATTACH_FILTER). The same
// holds for DLT_RAW (interfaces without a link header). Linux frames loopback
// traffic with an all-zero Ethernet header, while Darwin's lo0 is DLT_NULL: a
// 4-byte address family in host byte order. Those frames are rewritten here,
// and filters for them, being compiled for the DLT_NULL layout, always run in
// the interpreter in bpf_program.c.
//
// Opening the socket needs CAP_NET_RAW. Without it open() fails with EACCES,
// as it does on macOS for a user without access to /dev/bpf*.

#include <darling/emulation/xnu_syscall/bsd/helper/bpf/bpf_device.h>

#include <sys/errno.h>

#ifndef VARIANT_DYLD

#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/ioccom.h>
#include <sys/filio.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/uio.h>
#include <net/if.h>
#include <net/bpf.h>
#include <stdint.h>
#include <stddef.h>

#include <libsimple/lock.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/conversion/duct_errno.h>
#include <darling/emulation/conversion/fcntl/fcntl.h>
#include <darling/emulation/conversion/fcntl/open.h>
#include <darling/emulation/conversion/network/duct.h>
#include <darling/emulation/conversion/network/ifreq.h>
#include <darling/emulation/conversion/network/recvmsg.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/linux_premigration/vchroot_expand.h>
#include <darling/emulation/xnu_syscall/bsd/impl/ioctl/ioctl.h>
#include <darling/emulation/xnu_syscall/bsd/impl/mman/mman.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/close.h>
#include <darling/emulation/xnu_syscall/bsd/helper/misc/nofile_limit.h>

#include "bpf_program.h"

extern void* memcpy(void* dest, const void* src, __SIZE_TYPE__ n);
extern void* memset(void* dest, int c, __SIZE_TYPE__ n);
extern void* memmove(void* dest, const void* src, __SIZE_TYPE__ n);
extern int memcmp(const void* a, const void* b, __SIZE_TYPE__ n);
extern __SIZE_TYPE__ strlen(const char* s);

// Darwin ioctls declared in <net/bpf.h> only for PRIVATE builds.
#ifndef BIOCGETTC
#	define BIOCGETTC _IOR('B', 122, int)
#endif
#ifndef BIOCSETTC
#	define BIOCSETTC _IOW('B', 123, int)
#endif
#ifndef BIOCSEXTHDR
#	define BIOCSEXTHDR _IOW('B', 124, u_int)
#endif
#ifndef BIOCGIFATTACHCOUNT
#	define BIOCGIFATTACHCOUNT _IOWR('B', 125, struct ifreq)
#endif
#ifndef BIOCGWANTPKTAP
#	define BIOCGWANTPKTAP _IOR('B', 127, u_int)
#endif
#ifndef BIOCSWANTPKTAP
#	define BIOCSWANTPKTAP _IOWR('B', 127, u_int)
#endif
#ifndef BIOCSHEADDROP
#	define BIOCSHEADDROP _IOW('B', 128, int)
#endif
#ifndef BIOCGHEADDROP
#	define BIOCGHEADDROP _IOR('B', 128, int)
#endif
#ifndef BIOCSTRUNCATE
#	define BIOCSTRUNCATE _IOW('B', 129, u_int)
#endif
#ifndef BIOCSPKTHDRV2
#	define BIOCSPKTHDRV2 _IOW('B', 132, int)
#endif
#ifndef BIOCGPKTHDRV2
#	define BIOCGPKTHDRV2 _IOW('B', 133, int)
#endif

// The extended record header selected with BIOCSEXTHDR (Darwin's
// struct bpf_hdr_ext). The first fields are those of struct bpf_hdr.
struct bpf_ext_record_header {
	int32_t tstamp_sec;
	int32_t tstamp_usec;
	uint32_t caplen;
	uint32_t datalen;
	uint16_t hdrlen;
	uint16_t flags;
	int32_t pid;
	char comm[17];
	uint8_t pad;
	uint8_t pktflags;
	uint8_t proto;
	uint32_t svc;
	uint32_t flowid;
	uint32_t unsent_bytes;
	uint32_t unsent_snd;
};
_Static_assert(sizeof(struct bpf_ext_record_header) == 60, "bpf_hdr_ext is 60 bytes");
_Static_assert(offsetof(struct bpf_ext_record_header, flags) == 18, "bh_flags offset");
_Static_assert(offsetof(struct bpf_ext_record_header, svc) == 44, "bh_svc offset");
#define BPF_EXT_FLAG_DIR_OUT 0x0001

// struct bpf_hdr without its trailing padding (SIZEOF_BPF_HDR).
#define BPF_RECORD_HEADER_SIZE 18

// Defaults of XNU's debug.bpf_bufsize and debug.bpf_maxbufsize.
#define BPF_DEFAULT_BUFSIZE 4096
#define BPF_MAX_BUFSIZE BPF_MAXBUFSIZE

// st_rdev of an open device: a fixed major number of our own (XNU assigns
// the bpf major dynamically) and the unit number.
#define BPF_DEVICE_MAJOR 23

// Linux definitions (packet(7), socket(7)).
#define LINUX_AF_PACKET 17
#define LINUX_SOCK_RAW 3
#define LINUX_SOCK_NONBLOCK 04000
#define LINUX_SOCK_CLOEXEC 02000000
#define LINUX_ETH_P_ALL 0x0003
#define LINUX_ETH_P_IP 0x0800
#define LINUX_ETH_P_IPV6 0x86dd
#define LINUX_SOL_SOCKET 1
#define LINUX_SO_RCVBUF 8
#define LINUX_SO_ATTACH_FILTER 26
#define LINUX_SO_DETACH_FILTER 27
// Linux classic BPF ancillary load: 1 if the frame's VLAN tag was taken out.
#define LINUX_SKF_AD_VLAN_TAG_PRESENT (0xfffff000u + 48)
#define LINUX_SO_TIMESTAMP 29
#define LINUX_SCM_TIMESTAMP LINUX_SO_TIMESTAMP
#define LINUX_SOL_PACKET 263
#define LINUX_PACKET_ADD_MEMBERSHIP 1
#define LINUX_PACKET_DROP_MEMBERSHIP 2
#define LINUX_PACKET_STATISTICS 6
#define LINUX_PACKET_AUXDATA 8
#define LINUX_PACKET_IGNORE_OUTGOING 23
#define LINUX_PACKET_MR_PROMISC 1
#define LINUX_PACKET_OUTGOING 4
#define LINUX_TP_STATUS_VLAN_VALID 0x10
#define LINUX_TP_STATUS_VLAN_TPID_VALID 0x40
#define LINUX_ETH_P_8021Q 0x8100
#define LINUX_ARPHRD_ETHER 1
#define LINUX_ARPHRD_RAWIP 519
#define LINUX_ARPHRD_LOOPBACK 772
#define LINUX_ARPHRD_NONE 0xfffe
#define LINUX_SIOCGIFINDEX 0x8933
#define LINUX_SIOCINQ 0x541b
#define LINUX_POLLIN 0x0001
#define LINUX_CLOCK_REALTIME 0
#define LINUX_CLOCK_MONOTONIC 1

#define LINUX_SYS_SOCKET_CALL 1
#define LINUX_SYS_BIND_CALL 2
#define LINUX_SYS_SETSOCKOPT_CALL 14
#define LINUX_SYS_GETSOCKOPT_CALL 15
#define LINUX_SYS_SENDMSG_CALL 16
#define LINUX_SYS_RECVMSG_CALL 17

// Darwin address family values written into DLT_NULL headers.
#define DARWIN_AF_INET 2
#define DARWIN_AF_INET6 30

struct linux_sockaddr_ll {
	uint16_t sll_family;
	uint16_t sll_protocol; // network byte order
	int32_t sll_ifindex;
	uint16_t sll_hatype;
	uint8_t sll_pkttype;
	uint8_t sll_halen;
	uint8_t sll_addr[8];
};

struct linux_packet_mreq {
	int32_t mr_ifindex;
	uint16_t mr_type;
	uint16_t mr_alen;
	uint8_t mr_address[8];
};

// PACKET_AUXDATA control message payload.
struct linux_tpacket_auxdata {
	uint32_t tp_status;
	uint32_t tp_len;     // length before any truncation
	uint32_t tp_snaplen;
	uint16_t tp_mac;
	uint16_t tp_net;
	uint16_t tp_vlan_tci;
	uint16_t tp_vlan_tpid;
};

struct linux_tpacket_stats {
	uint32_t tp_packets;
	uint32_t tp_drops;
};

struct linux_sock_fprog {
	unsigned short len;
	const struct bpf_program_insn* filter;
};

struct linux_timespec {
	long tv_sec;
	long tv_nsec;
};

// SO_TIMESTAMP's control message payload: the old timeval of the process's
// ABI (two longs).
struct linux_timeval {
	long tv_sec;
	long tv_usec;
};

struct linux_pollfd {
	int fd;
	short events;
	short revents;
};

struct bpf_desc {
	int refs;
	// Protects everything below. Never held while blocking.
	libsimple_lock_t lock;

	int minor;
	int oflags;          // Darwin open(2) flags
	uint32_t blen;       // BIOCSBLEN; read() must use exactly this size

	// Interface (BIOCSETIF)
	int bound;
	int ifindex;
	char ifname[IFNAMSIZ];
	uint16_t hatype;
	uint32_t dlt;
	uint32_t linkhdr;    // link header length in records for this DLT
	uint8_t hwaddr[6];

	// Flags set with ioctls
	int promisc;
	int immediate;
	int seesent;
	int hdrcmplt;
	int exthdr;
	int wantpktap;
	int headdrop;
	int truncate;
	int pkthdrv2;
	int traffic_class;
	uint32_t signal;
	uint64_t rtout_usec; // 0: wait without a time limit

	// Filter (BIOCSETF); NULL accepts every packet whole.
	struct bpf_program_insn* filter;
	uint32_t filter_len;
	unsigned long filter_alloc;
	int filter_in_kernel;

	// Counters for BIOCGSTATS. Linux resets its counters on every read.
	uint32_t stat_recv;
	uint32_t stat_drop;

	// Buffers, allocated on the first BIOCSETIF (blen cannot change after).
	uint8_t* scratch;    // one received frame
	uint32_t scratch_size;
	uint8_t* hold;       // a record that did not fit into the last read
	uint32_t hold_len;
	unsigned long bufs_alloc;
};

#define BPF_MAX_FDS 256

struct bpf_fd_slot {
	int fd;
	struct bpf_desc* desc; // NULL: free
};

static struct bpf_fd_slot bpf_fds[BPF_MAX_FDS];
static libsimple_lock_t bpf_table_lock = LIBSIMPLE_LOCK_INITIALIZER;
volatile int bpf_device_fd_count = 0;

#define WORDALIGN(x) BPF_WORDALIGN(x)

// --- Linux socket calls ---------------------------------------------------

#ifdef __NR_socketcall
#	define SOCKCALL(call, nr, ...) LINUX_SYSCALL(__NR_socketcall, call, ((long[6]) { __VA_ARGS__ }))
#else
#	define SOCKCALL(call, nr, ...) LINUX_SYSCALL(nr, __VA_ARGS__)
#endif

static long l_socket(int domain, int type, int protocol)
{
	return SOCKCALL(LINUX_SYS_SOCKET_CALL, __NR_socket, domain, type, protocol);
}

static long l_bind(int fd, const void* addr, unsigned int len)
{
	return SOCKCALL(LINUX_SYS_BIND_CALL, __NR_bind, fd, (long) addr, len);
}

static long l_setsockopt(int fd, int level, int name, const void* val, unsigned int len)
{
	return SOCKCALL(LINUX_SYS_SETSOCKOPT_CALL, __NR_setsockopt, fd, level, name, (long) val, len);
}

static long l_getsockopt(int fd, int level, int name, void* val, unsigned int* len)
{
	return SOCKCALL(LINUX_SYS_GETSOCKOPT_CALL, __NR_getsockopt, fd, level, name, (long) val, (long) len);
}

static long l_sendmsg(int fd, const struct linux_msghdr* msg, int flags)
{
	return SOCKCALL(LINUX_SYS_SENDMSG_CALL, __NR_sendmsg, fd, (long) msg, flags);
}

static long l_recvmsg(int fd, struct linux_msghdr* msg, int flags)
{
	return SOCKCALL(LINUX_SYS_RECVMSG_CALL, __NR_recvmsg, fd, (long) msg, flags);
}

static int l_nonblocking(int fd)
{
#ifdef __NR_fcntl64
	long fl = LINUX_SYSCALL(__NR_fcntl64, fd, LINUX_F_GETFL);
#else
	long fl = LINUX_SYSCALL(__NR_fcntl, fd, LINUX_F_GETFL);
#endif
	return fl >= 0 && (fl & LINUX_O_NONBLOCK) != 0;
}

static uint64_t l_now_usec(int clock)
{
	struct linux_timespec ts = { 0, 0 };
	LINUX_SYSCALL(__NR_clock_gettime, clock, &ts);
	return (uint64_t) ts.tv_sec * 1000000 + (uint64_t) ts.tv_nsec / 1000;
}

static uint16_t to_be16(uint16_t v)
{
	return (uint16_t) ((v >> 8) | (v << 8));
}

// --- descriptor table -----------------------------------------------------

static struct bpf_desc* desc_alloc(void)
{
	void* mem = sys_mmap(NULL, sizeof(struct bpf_desc), PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
	if ((long) mem < 0 && (long) mem > -4096)
		return NULL;
	struct bpf_desc* d = mem;
	memset(d, 0, sizeof(*d));
	libsimple_lock_init(&d->lock);
	d->refs = 1;
	return d;
}

static void* buf_alloc(unsigned long size)
{
	void* mem = sys_mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
	if ((long) mem < 0 && (long) mem > -4096)
		return NULL;
	return mem;
}

static void desc_release(struct bpf_desc* d)
{
	if (__atomic_sub_fetch(&d->refs, 1, __ATOMIC_ACQ_REL) != 0)
		return;
	if (d->filter)
		sys_munmap(d->filter, d->filter_alloc);
	if (d->scratch)
		sys_munmap(d->scratch, d->bufs_alloc);
	sys_munmap(d, sizeof(*d));
}

// Returns the descriptor's state with a reference held, or NULL.
static struct bpf_desc* desc_lookup(int fd)
{
	if (__atomic_load_n(&bpf_device_fd_count, __ATOMIC_ACQUIRE) == 0 || fd < 0)
		return NULL;

	struct bpf_desc* found = NULL;
	libsimple_lock_lock(&bpf_table_lock);
	for (int i = 0; i < BPF_MAX_FDS; i++)
	{
		if (bpf_fds[i].desc && bpf_fds[i].fd == fd)
		{
			found = bpf_fds[i].desc;
			__atomic_add_fetch(&found->refs, 1, __ATOMIC_ACQ_REL);
			break;
		}
	}
	libsimple_lock_unlock(&bpf_table_lock);
	return found;
}

// Takes over one reference to d.
static int desc_register(int fd, struct bpf_desc* d)
{
	int ok = 0;
	libsimple_lock_lock(&bpf_table_lock);
	for (int i = 0; i < BPF_MAX_FDS; i++)
	{
		if (!bpf_fds[i].desc)
		{
			bpf_fds[i].fd = fd;
			bpf_fds[i].desc = d;
			__atomic_add_fetch(&bpf_device_fd_count, 1, __ATOMIC_ACQ_REL);
			ok = 1;
			break;
		}
	}
	libsimple_lock_unlock(&bpf_table_lock);
	return ok;
}

// Returns the reference the table held, or NULL.
static struct bpf_desc* desc_unregister(int fd)
{
	struct bpf_desc* d = NULL;
	libsimple_lock_lock(&bpf_table_lock);
	for (int i = 0; i < BPF_MAX_FDS; i++)
	{
		if (bpf_fds[i].desc && bpf_fds[i].fd == fd)
		{
			d = bpf_fds[i].desc;
			bpf_fds[i].desc = NULL;
			__atomic_sub_fetch(&bpf_device_fd_count, 1, __ATOMIC_ACQ_REL);
			break;
		}
	}
	libsimple_lock_unlock(&bpf_table_lock);
	return d;
}

void bpf_device_close(int fd)
{
	if (__atomic_load_n(&bpf_device_fd_count, __ATOMIC_ACQUIRE) == 0)
		return;
	struct bpf_desc* d = desc_unregister(fd);
	if (d)
		desc_release(d);
}

void bpf_device_dup(int oldfd, int newfd)
{
	if (oldfd == newfd)
		return;
	struct bpf_desc* d = desc_lookup(oldfd);
	if (!d)
		return;
	// The new descriptor shares the open file, as on XNU.
	if (!desc_register(newfd, d))
		desc_release(d);
}

void bpf_device_postfork_child(void)
{
	// Other threads of the parent may have held these locks; they do not
	// exist in the child.
	libsimple_lock_init(&bpf_table_lock);
	for (int i = 0; i < BPF_MAX_FDS; i++)
	{
		if (bpf_fds[i].desc)
			libsimple_lock_init(&bpf_fds[i].desc->lock);
	}
}

// --- open -----------------------------------------------------------------

int bpf_device_minor(const char* expanded_path)
{
	static const char suffix[] = "/dev/bpf";
	unsigned long len = strlen(expanded_path);

	// Cheap test on the host path first: it must end in "/dev/bpf<digits>".
	unsigned long digits = 0;
	while (digits < len && expanded_path[len - 1 - digits] >= '0' && expanded_path[len - 1 - digits] <= '9')
		digits++;
	if (digits == 0 || digits > 9 || len < digits + sizeof(suffix) - 1)
		return -1;
	if (memcmp(expanded_path + len - digits - (sizeof(suffix) - 1), suffix, sizeof(suffix) - 1) != 0)
		return -1;

	// Then check that it is the guest's /dev (which may lie outside the
	// prefix, under /Volumes/SystemRoot), not some other directory named dev.
	struct vchroot_unexpand_args args;
	if (len >= sizeof(args.path))
		return -1;
	memcpy(args.path, expanded_path, len + 1);
	if (vchroot_unexpand(&args) != 0)
		return -1;

	static const char system_root[] = "/Volumes/SystemRoot";
	const char* guest = args.path;
	if (memcmp(guest, system_root, sizeof(system_root) - 1) == 0 && guest[sizeof(system_root) - 1] == '/')
		guest += sizeof(system_root) - 1;
	if (strlen(guest) != sizeof(suffix) - 1 + digits || memcmp(guest, suffix, sizeof(suffix) - 1) != 0)
		return -1;

	int minor = 0;
	for (const char* p = guest + sizeof(suffix) - 1; *p; p++)
		minor = minor * 10 + (*p - '0');
	return minor;
}

static int minor_in_use(int minor)
{
	int busy = 0;
	libsimple_lock_lock(&bpf_table_lock);
	for (int i = 0; i < BPF_MAX_FDS; i++)
	{
		if (bpf_fds[i].desc && bpf_fds[i].desc->minor == minor)
		{
			busy = 1;
			break;
		}
	}
	libsimple_lock_unlock(&bpf_table_lock);
	return busy;
}

long bpf_device_open(int minor, int bsd_flags)
{
	// A unit can be open only once; callers try the next unit on EBUSY.
	// Only this process's units are known here, other processes' are not.
	if (minor_in_use(minor))
		return -EBUSY;

	int type = LINUX_SOCK_RAW;
	if (bsd_flags & O_NONBLOCK)
		type |= LINUX_SOCK_NONBLOCK;
	if (bsd_flags & O_CLOEXEC)
		type |= LINUX_SOCK_CLOEXEC;

	// Protocol 0: the socket receives nothing until BIOCSETIF binds it.
	long fd = l_socket(LINUX_AF_PACKET, type, 0);
	if (fd < 0)
	{
		// Packet sockets need CAP_NET_RAW; macOS denies access to the
		// device nodes with EACCES.
		if (fd == -LINUX_EPERM || fd == -LINUX_EACCES)
			return -EACCES;
		return errno_linux_to_bsd(fd);
	}

	fd = nofile_limit_check_new(fd);
	if (fd < 0)
		return fd;

	struct bpf_desc* d = desc_alloc();
	if (!d)
	{
		close_internal(fd);
		return -ENOMEM;
	}

	d->minor = minor;
	d->oflags = bsd_flags;
	d->blen = BPF_DEFAULT_BUFSIZE;
	d->seesent = 1;
	d->signal = SIGIO;

	// Receive timestamps, and the original length and VLAN tag of each
	// packet with it.
	int one = 1;
	l_setsockopt(fd, LINUX_SOL_SOCKET, LINUX_SO_TIMESTAMP, &one, sizeof(one));
	l_setsockopt(fd, LINUX_SOL_PACKET, LINUX_PACKET_AUXDATA, &one, sizeof(one));

	if (!desc_register(fd, d))
	{
		desc_release(d);
		close_internal(fd);
		return -ENFILE;
	}

	return fd;
}

// --- helpers for the bound socket -----------------------------------------

static uint32_t record_header_len(const struct bpf_desc* d)
{
	uint32_t size = d->exthdr ? sizeof(struct bpf_ext_record_header) : BPF_RECORD_HEADER_SIZE;
	// Pad so that the data after the link header is 32-bit aligned.
	return WORDALIGN(d->linkhdr + size) - d->linkhdr;
}

static void stats_collect_locked(int fd, struct bpf_desc* d)
{
	struct linux_tpacket_stats st = { 0, 0 };
	unsigned int len = sizeof(st);
	if (l_getsockopt(fd, LINUX_SOL_PACKET, LINUX_PACKET_STATISTICS, &st, &len) == 0)
	{
		// Linux counts dropped packets in tp_drops only; macOS's
		// bs_recv counts every packet the device saw.
		d->stat_recv += st.tp_packets + st.tp_drops;
		d->stat_drop += st.tp_drops;
	}
}

// Discards queued packets and the held record, and zeroes the counters
// (XNU's reset_d).
static void reset_locked(int fd, struct bpf_desc* d)
{
	d->hold_len = 0;
	if (d->bound && d->scratch)
	{
		struct iovec iov = { d->scratch, d->scratch_size };
		struct linux_msghdr msg;
		// Bounded, so a busy interface cannot keep us here.
		for (int i = 0; i < 4096; i++)
		{
			memset(&msg, 0, sizeof(msg));
			msg.msg_iov = &iov;
			msg.msg_iovlen = 1;
			if (l_recvmsg(fd, &msg, LINUX_MSG_DONTWAIT) < 0)
				break;
		}
	}
	stats_collect_locked(fd, d);
	d->stat_recv = 0;
	d->stat_drop = 0;
}

static void apply_filter_locked(int fd, struct bpf_desc* d)
{
	d->filter_in_kernel = 0;
	if (!d->bound)
		return;

	// The Linux kernel sees the same bytes as the program only for these
	// link types; DLT_NULL frames are rewritten here first.
	if (d->filter && (d->dlt == DLT_EN10MB || d->dlt == DLT_RAW))
	{
		// Linux takes 802.1Q tags out of Ethernet frames before the filter
		// runs. Frames that had one are put back together in read() and
		// filtered there; the kernel lets them through by the 3
		// instructions put in front of the program:
		//   ld vlan_tag_present; jeq #0, skip; ret #-1
		uint32_t extra = (d->dlt == DLT_EN10MB) ? 3 : 0;
		unsigned long size = (d->filter_len + extra) * sizeof(struct bpf_program_insn);
		struct bpf_program_insn* insns = buf_alloc(size);
		if (insns)
		{
			if (extra)
			{
				insns[0] = (struct bpf_program_insn) { 0x20 /* ld w abs */, 0, 0, LINUX_SKF_AD_VLAN_TAG_PRESENT };
				insns[1] = (struct bpf_program_insn) { 0x15 /* jeq k */, 1, 0, 0 };
				insns[2] = (struct bpf_program_insn) { 0x06 /* ret k */, 0, 0, 0xffffffffu };
			}
			memcpy(insns + extra, d->filter, d->filter_len * sizeof(struct bpf_program_insn));

			struct linux_sock_fprog prog = { (unsigned short) (d->filter_len + extra), insns };
			long rv = l_setsockopt(fd, LINUX_SOL_SOCKET, LINUX_SO_ATTACH_FILTER, &prog, sizeof(prog));
			sys_munmap(insns, size);
			if (rv == 0)
			{
				d->filter_in_kernel = 1;
				return;
			}
		}
	}

	// Let every packet through to the interpreter (fails harmlessly if no
	// kernel filter is attached). Linux wants an int-sized option value
	// even though it ignores it.
	int unused = 0;
	l_setsockopt(fd, LINUX_SOL_SOCKET, LINUX_SO_DETACH_FILTER, &unused, sizeof(unused));
}

static void apply_direction_locked(int fd, struct bpf_desc* d)
{
	if (!d->bound)
		return;
	// On loopback every packet also comes back in as received, so the
	// outgoing copy is always dropped. Elsewhere it is wanted unless
	// BIOCSSEESENT turned it off. Packets are filtered on direction in
	// read() as well; this only spares the kernel the work.
	int ignore = d->hatype == LINUX_ARPHRD_LOOPBACK || !d->seesent;
	l_setsockopt(fd, LINUX_SOL_PACKET, LINUX_PACKET_IGNORE_OUTGOING, &ignore, sizeof(ignore));
}

static int lookup_interface(int fd, const char* name, int* ifindex, uint16_t* hatype, uint8_t* hwaddr)
{
	struct linux_ifreq req;
	unsigned long n = strlen(name);
	if (n == 0 || n >= LINUX_IFNAMSIZ)
		return -ENXIO;

	memset(&req, 0, sizeof(req));
	memcpy(req.lifr_name, name, n);
	// __real_ioctl returns Darwin errnos.
	int rv = __real_ioctl(fd, LINUX_SIOCGIFINDEX, &req);
	if (rv < 0)
		return (rv == -ENODEV) ? -ENXIO : rv;
	*ifindex = req.lifr_ifindex;

	if (hatype)
	{
		memset(&req, 0, sizeof(req));
		memcpy(req.lifr_name, name, n);
		rv = __real_ioctl(fd, LINUX_SIOCGIFHWADDR, &req);
		if (rv < 0)
			return (rv == -ENODEV) ? -ENXIO : rv;
		*hatype = req.lifr_hwaddr.sa_family;
		memcpy(hwaddr, req.lifr_hwaddr.sa_data, 6);
	}
	return 0;
}

// Link types offered for a Linux hardware type; 0 if there is no mapping.
static int dlt_for_hatype(uint16_t hatype, uint32_t* dlt, uint32_t* linkhdr)
{
	switch (hatype)
	{
		case LINUX_ARPHRD_ETHER:
			*dlt = DLT_EN10MB;
			*linkhdr = 14;
			return 1;
		case LINUX_ARPHRD_LOOPBACK:
			*dlt = DLT_NULL;
			*linkhdr = 4;
			return 1;
		case LINUX_ARPHRD_NONE:
		case LINUX_ARPHRD_RAWIP:
			*dlt = DLT_RAW;
			*linkhdr = 0;
			return 1;
		default:
			return 0;
	}
}

static int set_interface_locked(int fd, struct bpf_desc* d, const char* name)
{
	int ifindex;
	uint16_t hatype;
	uint8_t hwaddr[6];
	uint32_t dlt, linkhdr;

	int rv = lookup_interface(fd, name, &ifindex, &hatype, hwaddr);
	if (rv < 0)
		return rv;
	// XNU fails with ENXIO for interfaces BPF cannot attach to.
	if (!dlt_for_hatype(hatype, &dlt, &linkhdr))
		return -ENXIO;

	if (!d->scratch)
	{
		// The largest frame kept is a record's worth of data plus the
		// Linux loopback header that is longer than DLT_NULL's.
		uint32_t scratch_size = d->blen + 16;
		unsigned long total = (unsigned long) scratch_size + d->blen;
		uint8_t* mem = buf_alloc(total);
		if (!mem)
			return -ENOBUFS;
		d->scratch = mem;
		d->scratch_size = scratch_size;
		d->hold = mem + scratch_size;
		d->bufs_alloc = total;
	}

	if (d->bound && d->promisc)
	{
		struct linux_packet_mreq mreq;
		memset(&mreq, 0, sizeof(mreq));
		mreq.mr_ifindex = d->ifindex;
		mreq.mr_type = LINUX_PACKET_MR_PROMISC;
		l_setsockopt(fd, LINUX_SOL_PACKET, LINUX_PACKET_DROP_MEMBERSHIP, &mreq, sizeof(mreq));
		d->promisc = 0;
	}

	struct linux_sockaddr_ll sll;
	memset(&sll, 0, sizeof(sll));
	sll.sll_family = LINUX_AF_PACKET;
	sll.sll_protocol = to_be16(LINUX_ETH_P_ALL);
	sll.sll_ifindex = ifindex;
	rv = l_bind(fd, &sll, sizeof(sll));
	if (rv < 0)
		return (rv == -LINUX_ENODEV) ? -ENXIO : errno_linux_to_bsd(rv);

	d->bound = 1;
	d->ifindex = ifindex;
	memset(d->ifname, 0, sizeof(d->ifname));
	memcpy(d->ifname, name, strlen(name));
	d->hatype = hatype;
	d->dlt = dlt;
	d->linkhdr = linkhdr;
	memcpy(d->hwaddr, hwaddr, sizeof(hwaddr));

	// Room for a full buffer of packets in the socket (best effort).
	int rcvbuf = (int) d->blen * 2;
	l_setsockopt(fd, LINUX_SOL_SOCKET, LINUX_SO_RCVBUF, &rcvbuf, sizeof(rcvbuf));

	apply_direction_locked(fd, d);
	apply_filter_locked(fd, d);
	reset_locked(fd, d);
	return 0;
}

static int set_filter_locked(int fd, struct bpf_desc* d, const struct bpf_program* prog, int flush)
{
	struct bpf_program_insn* insns = NULL;
	unsigned long alloc = 0;
	uint32_t count = prog->bf_len;

	if (prog->bf_insns == NULL)
	{
		if (count != 0)
			return -EINVAL;
	}
	else
	{
		if (count == 0 || count > BPF_PROGRAM_MAX_INSNS)
			return -EINVAL;
		alloc = count * sizeof(struct bpf_program_insn);
		insns = buf_alloc(alloc);
		if (!insns)
			return -ENOMEM;
		memcpy(insns, prog->bf_insns, alloc);
		if (!bpf_program_check(insns, count, BPF_MAX_BUFSIZE))
		{
			sys_munmap(insns, alloc);
			return -EINVAL;
		}
	}

	if (d->filter)
		sys_munmap(d->filter, d->filter_alloc);
	d->filter = insns;
	d->filter_len = insns ? count : 0;
	d->filter_alloc = alloc;

	apply_filter_locked(fd, d);
	if (flush)
		reset_locked(fd, d);
	return 0;
}

// --- read -----------------------------------------------------------------

struct bpf_capture {
	const uint8_t* data;
	uint32_t buflen;
	uint32_t wirelen;
	uint32_t snaplen;
	int outgoing;
	int vlan_restored;   // the kernel filter did not see this frame as is
	int32_t sec;
	int32_t usec;
};

// Turns a received Linux frame into what BPF delivers for the device's link
// type and runs the filter. Returns 0 if the packet is not wanted.
static int prepare_capture(struct bpf_desc* d, uint8_t* frame, uint32_t got, uint32_t wirelen,
	const struct linux_sockaddr_ll* from, struct bpf_capture* cap)
{
	cap->outgoing = from->sll_pkttype == LINUX_PACKET_OUTGOING;
	if (cap->outgoing && (d->hatype == LINUX_ARPHRD_LOOPBACK || !d->seesent))
		return 0;

	switch (d->dlt)
	{
		case DLT_NULL:
		{
			// Replace Linux's 14-byte all-zero Ethernet header with the
			// 4-byte address family that Darwin's loopback uses.
			if (got < 14)
				return 0;
			uint16_t ethertype = (uint16_t) ((frame[12] << 8) | frame[13]);
			uint32_t family;
			if (ethertype == LINUX_ETH_P_IP)
				family = DARWIN_AF_INET;
			else if (ethertype == LINUX_ETH_P_IPV6)
				family = DARWIN_AF_INET6;
			else
				return 0;
			memcpy(frame + 10, &family, sizeof(family));
			cap->data = frame + 10;
			cap->buflen = got - 10;
			cap->wirelen = wirelen - 10;
			break;
		}
		default:
			cap->data = frame;
			cap->buflen = got;
			cap->wirelen = wirelen;
			break;
	}

	cap->snaplen = 0xffffffffu;
	if (d->filter && (!d->filter_in_kernel || cap->vlan_restored))
	{
		cap->snaplen = bpf_program_run(d->filter, d->filter_len, cap->data, cap->wirelen, cap->buflen);
		if (cap->snaplen == 0)
			return 0;
	}
	return 1;
}

static void write_record(struct bpf_desc* d, uint8_t* dst, const struct bpf_capture* cap, uint32_t hdrlen, uint32_t caplen)
{
	memset(dst, 0, hdrlen);
	struct bpf_ext_record_header* h = (struct bpf_ext_record_header*) dst;
	h->tstamp_sec = cap->sec;
	h->tstamp_usec = cap->usec;
	h->caplen = caplen;
	h->datalen = cap->wirelen;
	h->hdrlen = (uint16_t) hdrlen;
	// Process attribution fields (pid, comm) are not available and stay 0.
	if (d->exthdr && cap->outgoing)
		h->flags = BPF_EXT_FLAG_DIR_OUT;
	memcpy(dst + hdrlen, cap->data, caplen);
}

// Receives one queued packet without blocking. On success, *frame points
// into the scratch buffer at the frame and the number of bytes there is
// returned (> 0); otherwise a negative Linux errno.
static long receive_frame(int fd, struct bpf_desc* d, uint8_t** frame, uint32_t* wirelen,
	struct linux_sockaddr_ll* from, struct bpf_capture* cap)
{
	// 4 bytes are kept free in front, to put back a VLAN tag.
	struct iovec iov = { d->scratch + 4, d->scratch_size - 4 };
	union {
		size_t align;
		char buf[128];
	} control;
	struct linux_msghdr msg;

	memset(&msg, 0, sizeof(msg));
	memset(from, 0, sizeof(*from));
	msg.msg_name = from;
	msg.msg_namelen = sizeof(*from);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = &control;
	msg.msg_controllen = sizeof(control);

	// With MSG_TRUNC, packet sockets return the frame's length even when it
	// did not fit; that is still after a kernel filter's truncation, so the
	// original length comes from PACKET_AUXDATA.
	long n = l_recvmsg(fd, &msg, LINUX_MSG_DONTWAIT | LINUX_MSG_TRUNC);
	if (n < 0)
		return n;

	*frame = d->scratch + 4;
	*wirelen = (uint32_t) n;
	cap->vlan_restored = 0;
	uint32_t got = (uint32_t) n < iov.iov_len ? (uint32_t) n : (uint32_t) iov.iov_len;

	int have_time = 0;
	struct linux_tpacket_auxdata aux;
	int have_aux = 0;
	if (msg.msg_controllen >= sizeof(struct linux_cmsghdr))
	{
		char* p = control.buf;
		char* end = control.buf + msg.msg_controllen;
		while (p + sizeof(struct linux_cmsghdr) <= end)
		{
			struct linux_cmsghdr* c = (struct linux_cmsghdr*) p;
			if (c->cmsg_len < sizeof(struct linux_cmsghdr) || p + c->cmsg_len > end)
				break;
			if (c->cmsg_level == LINUX_SOL_SOCKET && c->cmsg_type == LINUX_SCM_TIMESTAMP
				&& c->cmsg_len >= LINUX_CMSG_LEN(sizeof(struct linux_timeval)))
			{
				struct linux_timeval tv;
				memcpy(&tv, p + LINUX_CMSG_LEN(0), sizeof(tv));
				cap->sec = (int32_t) tv.tv_sec;
				cap->usec = (int32_t) tv.tv_usec;
				have_time = 1;
			}
			else if (c->cmsg_level == LINUX_SOL_PACKET && c->cmsg_type == LINUX_PACKET_AUXDATA
				&& c->cmsg_len >= LINUX_CMSG_LEN(sizeof(aux)))
			{
				memcpy(&aux, p + LINUX_CMSG_LEN(0), sizeof(aux));
				have_aux = 1;
			}
			p += LINUX_CMSG_ALIGN(c->cmsg_len);
		}
	}
	if (!have_time)
	{
		uint64_t now = l_now_usec(LINUX_CLOCK_REALTIME);
		cap->sec = (int32_t) (now / 1000000);
		cap->usec = (int32_t) (now % 1000000);
	}

	if (have_aux)
	{
		if (aux.tp_len > *wirelen)
			*wirelen = aux.tp_len;

		// Linux takes the 802.1Q tag out of Ethernet frames; BPF shows
		// the frame as it was on the wire.
		if ((aux.tp_status & LINUX_TP_STATUS_VLAN_VALID) && d->dlt == DLT_EN10MB && got >= 12)
		{
			uint16_t tpid = (aux.tp_status & LINUX_TP_STATUS_VLAN_TPID_VALID) ? aux.tp_vlan_tpid : LINUX_ETH_P_8021Q;
			uint8_t* f = d->scratch;
			memmove(f, f + 4, 12);
			f[12] = (uint8_t) (tpid >> 8);
			f[13] = (uint8_t) tpid;
			f[14] = (uint8_t) (aux.tp_vlan_tci >> 8);
			f[15] = (uint8_t) aux.tp_vlan_tci;
			*frame = f;
			got += 4;
			*wirelen += 4;
			cap->vlan_restored = 1;
		}
	}

	return got;
}

// Moves queued packets into buf (whose records end at *end) until the
// buffer is full or nothing more is queued. Returns 1 if the buffer is
// full, 0 if the socket has nothing more, or a negative Darwin errno.
static int fill_locked(int fd, struct bpf_desc* d, uint8_t* buf, uint32_t* end)
{
	if (d->hold_len)
	{
		uint32_t start = WORDALIGN(*end);
		if (start + d->hold_len > d->blen)
			return 1;
		memcpy(buf + start, d->hold, d->hold_len);
		*end = start + d->hold_len;
		d->hold_len = 0;
	}

	if (!d->bound)
		return 0;

	uint32_t hdrlen = record_header_len(d);
	for (;;)
	{
		struct linux_sockaddr_ll from;
		struct bpf_capture cap;
		uint32_t wirelen;
		uint8_t* frame;

		long got = receive_frame(fd, d, &frame, &wirelen, &from, &cap);
		if (got == -LINUX_EAGAIN)
			return 0;
		if (got == -LINUX_ENETDOWN)
			continue; // reported once when the interface goes down
		if (got < 0)
			return errno_linux_to_bsd(got);

		if (!prepare_capture(d, frame, (uint32_t) got, wirelen, &from, &cap))
			continue;

		if (hdrlen >= d->blen)
			continue;
		uint32_t caplen = cap.buflen;
		if (cap.snaplen < caplen)
			caplen = cap.snaplen;
		// A record never exceeds the buffer; the packet is truncated.
		if (hdrlen + caplen > d->blen)
			caplen = d->blen - hdrlen;

		uint32_t reclen = hdrlen + caplen;
		uint32_t start = WORDALIGN(*end);
		if (start + reclen > d->blen)
		{
			// Keep it for the next read.
			write_record(d, d->hold, &cap, hdrlen, caplen);
			d->hold_len = reclen;
			return 1;
		}
		write_record(d, buf + start, &cap, hdrlen, caplen);
		*end = start + reclen;
	}
}

static long bpf_read(int fd, struct bpf_desc* d, uint8_t* buf, uint32_t len)
{
	if ((d->oflags & O_ACCMODE) == O_WRONLY)
		return -EBADF;

	libsimple_lock_lock(&d->lock);
	uint32_t blen = d->blen;
	uint64_t rtout = d->rtout_usec;
	libsimple_lock_unlock(&d->lock);

	// bpf(4): the read size must equal the buffer size.
	if (len != blen)
		return -EINVAL;

	int nonblocking = l_nonblocking(fd);
	uint64_t deadline = rtout ? l_now_usec(LINUX_CLOCK_MONOTONIC) + rtout : 0;
	uint32_t end = 0;

	for (;;)
	{
		libsimple_lock_lock(&d->lock);
		int bound = d->bound;
		int immediate = d->immediate;
		int full = fill_locked(fd, d, buf, &end);
		libsimple_lock_unlock(&d->lock);

		if (full < 0)
			return end ? (long) end : full;
		if (full)
			return end;
		// Immediate and non-blocking reads return whatever is there.
		if (end && (immediate || nonblocking))
			return end;
		if (!end && !bound)
			return -ENXIO;
		if (!end && nonblocking)
			return -EAGAIN;

		struct linux_timespec ts, *tsp = NULL;
		if (rtout)
		{
			uint64_t now = l_now_usec(LINUX_CLOCK_MONOTONIC);
			// On timeout, return what has been collected, maybe nothing.
			if (now >= deadline)
				return end;
			uint64_t left = deadline - now;
			ts.tv_sec = (long) (left / 1000000);
			ts.tv_nsec = (long) (left % 1000000) * 1000;
			tsp = &ts;
		}

		struct linux_pollfd pfd = { fd, LINUX_POLLIN, 0 };
		long rv = LINUX_SYSCALL(__NR_ppoll, &pfd, 1, tsp, NULL, 0);
		if (rv == -LINUX_EINTR)
			return end ? (long) end : -EINTR;
		if (rv < 0)
			return end ? (long) end : errno_linux_to_bsd(rv);
		if (rv == 0 && rtout)
			return end;
	}
}

// --- write ----------------------------------------------------------------

static long bpf_write(int fd, struct bpf_desc* d, const uint8_t* buf, uint32_t len)
{
	if ((d->oflags & O_ACCMODE) == O_RDONLY)
		return -EBADF;

	libsimple_lock_lock(&d->lock);
	int bound = d->bound;
	int ifindex = d->ifindex;
	uint32_t dlt = d->dlt;
	int hdrcmplt = d->hdrcmplt;
	uint8_t hwaddr[6];
	memcpy(hwaddr, d->hwaddr, sizeof(hwaddr));
	libsimple_lock_unlock(&d->lock);

	if (!bound)
		return -ENXIO;
	if (len == 0)
		return 0;

	struct linux_sockaddr_ll to;
	memset(&to, 0, sizeof(to));
	to.sll_family = LINUX_AF_PACKET;
	to.sll_ifindex = ifindex;

	struct iovec iov[3];
	int iovcnt;
	uint8_t ethhdr[14];

	switch (dlt)
	{
		case DLT_EN10MB:
			if (len < 14)
				return -EINVAL;
			to.sll_protocol = (uint16_t) (buf[12] | (buf[13] << 8));
			if (hdrcmplt)
			{
				iov[0].iov_base = (void*) buf;
				iov[0].iov_len = len;
				iovcnt = 1;
			}
			else
			{
				// Without BIOCSHDRCMPLT the interface supplies the
				// source address.
				iov[0].iov_base = (void*) buf;
				iov[0].iov_len = 6;
				iov[1].iov_base = hwaddr;
				iov[1].iov_len = 6;
				iov[2].iov_base = (void*) (buf + 12);
				iov[2].iov_len = len - 12;
				iovcnt = 3;
			}
			break;

		case DLT_NULL:
		{
			// A 4-byte address family in host byte order, then the packet.
			uint32_t family;
			uint16_t ethertype;
			if (len < 4)
				return -EINVAL;
			memcpy(&family, buf, sizeof(family));
			if (family == DARWIN_AF_INET)
				ethertype = LINUX_ETH_P_IP;
			else if (family == DARWIN_AF_INET6)
				ethertype = LINUX_ETH_P_IPV6;
			else
				return -EAFNOSUPPORT;
			memset(ethhdr, 0, sizeof(ethhdr));
			ethhdr[12] = (uint8_t) (ethertype >> 8);
			ethhdr[13] = (uint8_t) ethertype;
			to.sll_protocol = to_be16(ethertype);
			iov[0].iov_base = ethhdr;
			iov[0].iov_len = sizeof(ethhdr);
			iov[1].iov_base = (void*) (buf + 4);
			iov[1].iov_len = len - 4;
			iovcnt = 2;
			break;
		}

		case DLT_RAW:
		default:
		{
			uint8_t version = buf[0] >> 4;
			if (version == 4)
				to.sll_protocol = to_be16(LINUX_ETH_P_IP);
			else if (version == 6)
				to.sll_protocol = to_be16(LINUX_ETH_P_IPV6);
			else
				return -EAFNOSUPPORT;
			iov[0].iov_base = (void*) buf;
			iov[0].iov_len = len;
			iovcnt = 1;
			break;
		}
	}

	struct linux_msghdr msg;
	memset(&msg, 0, sizeof(msg));
	msg.msg_name = &to;
	msg.msg_namelen = sizeof(to);
	msg.msg_iov = iov;
	msg.msg_iovlen = iovcnt;

	long rv = l_sendmsg(fd, &msg, 0);
	if (rv < 0)
		return errno_linux_to_bsd(rv);
	return len;
}

// --- ioctl ----------------------------------------------------------------

static int bpf_ioctl(int fd, struct bpf_desc* d, unsigned int cmd, void* arg)
{
	if (cmd != BIOCFLUSH && cmd != BIOCPROMISC && arg == NULL)
		return -EFAULT;

	switch (cmd)
	{
		case FIONREAD:
		{
			int n = d->hold_len;
			int queued = 0;
			// Linux reports the size of the next queued frame.
			if (__real_ioctl(fd, LINUX_SIOCINQ, &queued) == 0 && queued > 0 && d->bound)
				n += record_header_len(d) + queued;
			*(int*) arg = n;
			return 0;
		}

		case BIOCGBLEN:
			*(u_int*) arg = d->blen;
			return 0;

		case BIOCSBLEN:
		{
			// The size is fixed once an interface is attached.
			if (d->bound)
				return -EINVAL;
			u_int size = *(u_int*) arg;
			if (size > BPF_MAX_BUFSIZE)
				size = BPF_MAX_BUFSIZE;
			else if (size < BPF_MINBUFSIZE)
				size = BPF_MINBUFSIZE;
			d->blen = size;
			*(u_int*) arg = size;
			return 0;
		}

		case BIOCSETF:
		case BIOCSETFNR:
			return set_filter_locked(fd, d, (const struct bpf_program*) arg, cmd == BIOCSETF);

		case BIOCFLUSH:
			reset_locked(fd, d);
			return 0;

		case BIOCPROMISC:
		{
			if (!d->bound)
				return -EINVAL;
			if (d->promisc)
				return 0;
			struct linux_packet_mreq mreq;
			memset(&mreq, 0, sizeof(mreq));
			mreq.mr_ifindex = d->ifindex;
			mreq.mr_type = LINUX_PACKET_MR_PROMISC;
			long rv = l_setsockopt(fd, LINUX_SOL_PACKET, LINUX_PACKET_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
			if (rv < 0)
				return errno_linux_to_bsd(rv);
			d->promisc = 1;
			return 0;
		}

		case BIOCGDLT:
			if (!d->bound)
				return -EINVAL;
			*(u_int*) arg = d->dlt;
			return 0;

		case BIOCGDLTLIST:
		{
			if (!d->bound)
				return -EINVAL;
			struct bpf_dltlist* list = arg;
			// One link type per interface.
			if (list->bfl_list != NULL)
			{
				if (list->bfl_len < 1)
					return -ENOMEM;
				list->bfl_list[0] = d->dlt;
			}
			list->bfl_len = 1;
			return 0;
		}

		case BIOCSDLT:
			if (!d->bound)
				return -EINVAL;
			if (*(u_int*) arg != d->dlt)
				return -EINVAL;
			return 0;

		case BIOCGETIF:
		{
			if (!d->bound)
				return -EINVAL;
			struct ifreq* ifr = arg;
			memset(ifr->ifr_name, 0, sizeof(ifr->ifr_name));
			memcpy(ifr->ifr_name, d->ifname, strlen(d->ifname));
			return 0;
		}

		case BIOCSETIF:
		{
			char name[IFNAMSIZ];
			memcpy(name, ((const struct ifreq*) arg)->ifr_name, IFNAMSIZ);
			name[IFNAMSIZ - 1] = '\0';
			return set_interface_locked(fd, d, name);
		}

		case BIOCGIFATTACHCOUNT:
		{
			// Counts the descriptors of this process attached to the
			// interface; other processes' are not visible.
			struct ifreq* ifr = arg;
			char name[IFNAMSIZ];
			int ifindex;
			memcpy(name, ifr->ifr_name, IFNAMSIZ);
			name[IFNAMSIZ - 1] = '\0';
			int rv = lookup_interface(fd, name, &ifindex, NULL, NULL);
			if (rv < 0)
				return rv;
			int count = 0;
			libsimple_lock_lock(&bpf_table_lock);
			for (int i = 0; i < BPF_MAX_FDS; i++)
			{
				struct bpf_desc* other = bpf_fds[i].desc;
				if (other && other->bound && other->ifindex == ifindex)
					count++;
			}
			libsimple_lock_unlock(&bpf_table_lock);
			memcpy((char*) ifr + offsetof(struct ifreq, ifr_ifru), &count, sizeof(count));
			return 0;
		}

		case BIOCSRTIMEOUT:
		{
			const struct timeval* tv = arg;
			if (tv->tv_sec < 0 || tv->tv_usec < 0 || tv->tv_usec >= 1000000 || tv->tv_sec > 100000000)
				return -EINVAL;
			d->rtout_usec = (uint64_t) tv->tv_sec * 1000000 + (uint64_t) tv->tv_usec;
			return 0;
		}

		case BIOCGRTIMEOUT:
		{
			struct timeval* tv = arg;
			memset(tv, 0, sizeof(*tv));
			tv->tv_sec = (long) (d->rtout_usec / 1000000);
			tv->tv_usec = (int) (d->rtout_usec % 1000000);
			return 0;
		}

		case BIOCGSTATS:
		{
			struct bpf_stat* st = arg;
			stats_collect_locked(fd, d);
			st->bs_recv = d->stat_recv;
			st->bs_drop = d->stat_drop;
			return 0;
		}

		case BIOCIMMEDIATE:
			d->immediate = *(u_int*) arg;
			return 0;

		case BIOCVERSION:
		{
			struct bpf_version* v = arg;
			v->bv_major = BPF_MAJOR_VERSION;
			v->bv_minor = BPF_MINOR_VERSION;
			return 0;
		}

		case BIOCGHDRCMPLT:
			*(u_int*) arg = d->hdrcmplt;
			return 0;
		case BIOCSHDRCMPLT:
			d->hdrcmplt = *(u_int*) arg ? 1 : 0;
			return 0;

		case BIOCGSEESENT:
			*(u_int*) arg = d->seesent;
			return 0;
		case BIOCSSEESENT:
			d->seesent = *(u_int*) arg;
			apply_direction_locked(fd, d);
			return 0;

		case BIOCGRSIG:
			*(u_int*) arg = d->signal;
			return 0;
		case BIOCSRSIG:
		{
			u_int sig = *(u_int*) arg;
			if (sig >= __DARWIN_NSIG)
				return -EINVAL;
			d->signal = sig;
			return 0;
		}

		case BIOCGETTC:
			*(int*) arg = d->traffic_class;
			return 0;
		case BIOCSETTC:
			d->traffic_class = *(int*) arg;
			return 0;

		case BIOCSEXTHDR:
			d->exthdr = *(u_int*) arg ? 1 : 0;
			return 0;

		// Apple's pktap pseudo-interfaces do not exist here; selecting one
		// with BIOCSETIF fails with ENXIO, and libpcap falls back to a
		// plain interface. The flags themselves are kept.
		case BIOCGWANTPKTAP:
			*(u_int*) arg = d->wantpktap;
			return 0;
		case BIOCSWANTPKTAP:
			d->wantpktap = *(u_int*) arg ? 1 : 0;
			*(u_int*) arg = d->wantpktap;
			return 0;
		case BIOCSTRUNCATE:
			d->truncate = *(u_int*) arg ? 1 : 0;
			return 0;
		case BIOCSPKTHDRV2:
			d->pkthdrv2 = *(int*) arg ? 1 : 0;
			return 0;
		case BIOCGPKTHDRV2:
			*(int*) arg = d->pkthdrv2;
			return 0;

		// Head drop selects which packets are lost when the buffer
		// overflows; Linux always drops the newest. Kept for BIOCGHEADDROP.
		case BIOCSHEADDROP:
			d->headdrop = *(int*) arg ? 1 : 0;
			return 0;
		case BIOCGHEADDROP:
			*(int*) arg = d->headdrop;
			return 0;

		default:
			return -EINVAL;
	}
}

// --- entry points ---------------------------------------------------------

int bpf_device_read(int fd, void* buf, int len, long* retval)
{
	struct bpf_desc* d = desc_lookup(fd);
	if (!d)
		return 0;
	*retval = (len < 0) ? -EINVAL : bpf_read(fd, d, buf, (uint32_t) len);
	desc_release(d);
	return 1;
}

int bpf_device_write(int fd, const void* buf, int len, long* retval)
{
	struct bpf_desc* d = desc_lookup(fd);
	if (!d)
		return 0;
	*retval = (len < 0) ? -EINVAL : bpf_write(fd, d, buf, (uint32_t) len);
	desc_release(d);
	return 1;
}

int bpf_device_ioctl(int fd, unsigned int cmd, void* arg, int* retval)
{
	// Generic descriptor ioctls (FIONBIO, FIOASYNC, FIOCLEX...) apply to the
	// socket underneath; only BPF's own and FIONREAD are handled here.
	if (IOCGROUP(cmd) != 'B' && cmd != FIONREAD)
		return IOCTL_PASS;

	struct bpf_desc* d = desc_lookup(fd);
	if (!d)
		return IOCTL_PASS;

	libsimple_lock_lock(&d->lock);
	*retval = bpf_ioctl(fd, d, cmd, arg);
	libsimple_lock_unlock(&d->lock);
	desc_release(d);
	return IOCTL_HANDLED;
}

int bpf_device_stat(int fd, unsigned int* mode, int* rdev)
{
	struct bpf_desc* d = desc_lookup(fd);
	if (!d)
		return 0;
	*mode = S_IFCHR | S_IRUSR | S_IWUSR;
	*rdev = (BPF_DEVICE_MAJOR << 24) | (d->minor & 0xffffff);
	desc_release(d);
	return 1;
}

#else // VARIANT_DYLD

// dyld does not open devices.
volatile int bpf_device_fd_count = 0;

int bpf_device_minor(const char* expanded_path) { return -1; }
long bpf_device_open(int minor, int bsd_flags) { return -ENOENT; }
void bpf_device_close(int fd) { }
void bpf_device_dup(int oldfd, int newfd) { }
void bpf_device_postfork_child(void) { }
int bpf_device_read(int fd, void* buf, int len, long* retval) { return 0; }
int bpf_device_write(int fd, const void* buf, int len, long* retval) { return 0; }
int bpf_device_ioctl(int fd, unsigned int cmd, void* arg, int* retval) { return 0; }
int bpf_device_stat(int fd, unsigned int* mode, int* rdev) { return 0; }

#endif // VARIANT_DYLD
