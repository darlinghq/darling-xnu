/*
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

#include <darling/emulation/conversion/network/cmsg.h>

#include <sys/errno.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/network/getsockopt.h>
#include <darling/emulation/conversion/network/recvmsg.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

extern void* memcpy(void* dest, const void* src, __SIZE_TYPE__ len);
extern void* memset(void* dest, int c, __SIZE_TYPE__ len);

// Darwin numbering (<sys/socket.h>, <netinet/in.h>, <netinet6/in6.h>).
#define BSD_SCM_RIGHTS          1
#define BSD_SCM_TIMESTAMP       2
#define BSD_IP_TOS              3
#define BSD_IP_RECVTTL_CMSG     24
#define BSD_IP_PKTINFO          26
#define BSD_IP_RECVTOS          27
#define BSD_IPV6_2292PKTINFO_C  19
#define BSD_IPV6_2292HOPLIMIT_C 20
#define BSD_IPV6_TCLASS         36
#define BSD_IPV6_PKTINFO        46
#define BSD_IPV6_HOPLIMIT       47

// Linux numbering (<asm-generic/socket.h>, <linux/in.h>, <linux/in6.h>).
#define LINUX_SCM_RIGHTS        1
#define LINUX_SCM_TIMESTAMP     29 // SO_TIMESTAMP_OLD, struct timeval
#define LINUX_IP_TOS_C          1
#define LINUX_IP_TTL_C          2
#define LINUX_IP_PKTINFO        8
#define LINUX_IPV6_2292PKTINFO_C  2
#define LINUX_IPV6_2292HOPLIMIT_C 8
#define LINUX_IPV6_PKTINFO      50
#define LINUX_IPV6_HOPLIMIT     52
#define LINUX_IPV6_TCLASS       67

// The largest Darwin receive buffer we size a Linux buffer for. Linux caps
// SCM_RIGHTS at 253 descriptors per message and the other types are a few
// bytes each, so anything beyond this would only ever stay empty.
#define CMSG_RECV_MAX           (32 * 1024)

enum cmsg_kind
{
	KIND_COPY,         // payload has the same layout on both sides
	KIND_FDS,          // SCM_RIGHTS: array of int
	KIND_INT_TO_BYTE,  // Linux delivers an int where Darwin delivers one byte
};

enum
{
	DIR_TO_LINUX = 1,
	DIR_TO_BSD = 2,
	DIR_BOTH = 3,
};

struct cmsg_map
{
	int bsd_level, bsd_type;
	int linux_level, linux_type;
	unsigned char kind;
	unsigned char dir;
};

static const struct cmsg_map cmsg_maps[] = {
	{ BSD_SOL_SOCKET, BSD_SCM_RIGHTS, LINUX_SOL_SOCKET, LINUX_SCM_RIGHTS, KIND_FDS, DIR_BOTH },
	{ BSD_SOL_SOCKET, BSD_SCM_TIMESTAMP, LINUX_SOL_SOCKET, LINUX_SCM_TIMESTAMP, KIND_COPY, DIR_TO_BSD },

	// IP_PKTINFO: struct in_pktinfo is identical on both.
	{ XNU_IPPROTO_IP, BSD_IP_PKTINFO, XNU_IPPROTO_IP, LINUX_IP_PKTINFO, KIND_COPY, DIR_BOTH },
	// Sending: IP_TOS sets the TOS of this datagram. Receiving: Darwin reports
	// IP_RECVTOS with one byte, Linux reports IP_TOS with one byte.
	{ XNU_IPPROTO_IP, BSD_IP_TOS, XNU_IPPROTO_IP, LINUX_IP_TOS_C, KIND_COPY, DIR_TO_LINUX },
	{ XNU_IPPROTO_IP, BSD_IP_RECVTOS, XNU_IPPROTO_IP, LINUX_IP_TOS_C, KIND_COPY, DIR_TO_BSD },
	// Darwin reports IP_RECVTTL with one byte, Linux reports IP_TTL as an int.
	{ XNU_IPPROTO_IP, BSD_IP_RECVTTL_CMSG, XNU_IPPROTO_IP, LINUX_IP_TTL_C, KIND_INT_TO_BYTE, DIR_TO_BSD },

	// RFC 3542 and RFC 2292 IPv6 types; the payloads are identical.
	{ XNU_IPPROTO_IPV6, BSD_IPV6_PKTINFO, XNU_IPPROTO_IPV6, LINUX_IPV6_PKTINFO, KIND_COPY, DIR_BOTH },
	{ XNU_IPPROTO_IPV6, BSD_IPV6_HOPLIMIT, XNU_IPPROTO_IPV6, LINUX_IPV6_HOPLIMIT, KIND_COPY, DIR_BOTH },
	{ XNU_IPPROTO_IPV6, BSD_IPV6_TCLASS, XNU_IPPROTO_IPV6, LINUX_IPV6_TCLASS, KIND_COPY, DIR_BOTH },
	{ XNU_IPPROTO_IPV6, BSD_IPV6_2292PKTINFO_C, XNU_IPPROTO_IPV6, LINUX_IPV6_2292PKTINFO_C, KIND_COPY, DIR_BOTH },
	{ XNU_IPPROTO_IPV6, BSD_IPV6_2292HOPLIMIT_C, XNU_IPPROTO_IPV6, LINUX_IPV6_2292HOPLIMIT_C, KIND_COPY, DIR_BOTH },
};

#define BSD_HDR_LEN   BSD_CMSG_LEN(0)
#define LINUX_HDR_LEN LINUX_CMSG_LEN(0)

enum lookup_result
{
	LOOKUP_MAPPED,
	LOOKUP_PASS,   // level we do not translate: same numbers on both sides
	LOOKUP_DROP,   // type with no counterpart on the other side
	LOOKUP_INVALID,
};

static int is_translated_level_bsd(int level)
{
	return level == BSD_SOL_SOCKET || level == XNU_IPPROTO_IP || level == XNU_IPPROTO_IPV6;
}

static int is_translated_level_linux(int level)
{
	return level == LINUX_SOL_SOCKET || level == XNU_IPPROTO_IP || level == XNU_IPPROTO_IPV6;
}

static enum lookup_result lookup_bsd(int level, int type, const struct cmsg_map** out)
{
	for (size_t i = 0; i < sizeof(cmsg_maps) / sizeof(cmsg_maps[0]); i++)
	{
		const struct cmsg_map* m = &cmsg_maps[i];
		if ((m->dir & DIR_TO_LINUX) && m->bsd_level == level && m->bsd_type == type)
		{
			*out = m;
			return LOOKUP_MAPPED;
		}
	}

	if (!is_translated_level_bsd(level))
		return LOOKUP_PASS;

	// macOS rejects every SOL_SOCKET type but SCM_RIGHTS with EINVAL on
	// AF_UNIX sockets (SCM_CREDS, SCM_TIMESTAMP and unknown types alike).
	// Passing one on would also hand Linux a type number that means
	// something else there (Darwin SCM_TIMESTAMP is Linux SCM_CREDENTIALS).
	if (level == BSD_SOL_SOCKET)
		return LOOKUP_INVALID;

	// macOS ignores IP-level types it does not act on (IP_TTL, unknown
	// numbers) when sending a datagram, so leave them out rather than let
	// Linux reject or misinterpret them.
	return LOOKUP_DROP;
}

static enum lookup_result lookup_linux(int level, int type, const struct cmsg_map** out)
{
	for (size_t i = 0; i < sizeof(cmsg_maps) / sizeof(cmsg_maps[0]); i++)
	{
		const struct cmsg_map* m = &cmsg_maps[i];
		if ((m->dir & DIR_TO_BSD) && m->linux_level == level && m->linux_type == type)
		{
			*out = m;
			return LOOKUP_MAPPED;
		}
	}

	if (!is_translated_level_linux(level))
		return LOOKUP_PASS;

	// Linux-only types (SCM_CREDENTIALS, SCM_TIMESTAMPNS, IP_ORIGDSTADDR...)
	// can only show up if something enabled them behind the program's back;
	// a Darwin program could not make sense of them.
	return LOOKUP_DROP;
}

// Advances *off past a message of len bytes padded to align, without wrapping
// around on a cmsg_len close to the top of the address space.
static void advance(size_t* off, size_t total, size_t len, size_t align)
{
	size_t pad = (align - (len & (align - 1))) & (align - 1);
	size_t left = total - *off;

	if (len >= left || pad >= left - len)
		*off = total;
	else
		*off += len + pad;
}

static size_t bsd_payload_out_len(const struct cmsg_map* m, size_t linux_len)
{
	if (m != NULL && m->kind == KIND_INT_TO_BYTE && linux_len >= sizeof(int))
		return 1;
	return linux_len;
}

long cmsg_bsd_to_linux_size(const void* bsd_control, size_t bsd_len)
{
	const unsigned char* p = (const unsigned char*) bsd_control;
	size_t off = 0, out = 0;

	// macOS fails sendmsg() with EINVAL whenever msg_control is set and
	// msg_controllen cannot hold a header, on every socket type.
	if (bsd_len < BSD_HDR_LEN)
		return -EINVAL;

	while (bsd_len - off >= BSD_HDR_LEN)
	{
		struct bsd_cmsghdr h;
		const struct cmsg_map* m = NULL;
		size_t dlen;

		memcpy(&h, p + off, sizeof(h));

		if (h.cmsg_len < BSD_HDR_LEN || h.cmsg_len > bsd_len - off)
			return -EINVAL;
		dlen = h.cmsg_len - BSD_HDR_LEN;

		switch (lookup_bsd(h.cmsg_level, h.cmsg_type, &m))
		{
			case LOOKUP_INVALID:
				return -EINVAL;
			case LOOKUP_DROP:
				break;
			case LOOKUP_MAPPED:
				if (m->kind == KIND_FDS && (dlen % sizeof(int)) != 0)
					return -EINVAL;
				// fall through
			case LOOKUP_PASS:
				out += LINUX_CMSG_SPACE(dlen);
				break;
		}

		advance(&off, bsd_len, h.cmsg_len, sizeof(unsigned int));
	}

	// Bytes after the last message that cannot even hold a header: macOS
	// rejects these with EINVAL, and so does Linux for its own layout.
	if (off != bsd_len)
		return -EINVAL;

	return (long) out;
}

size_t cmsg_bsd_to_linux(const void* bsd_control, size_t bsd_len, void* linux_control)
{
	const unsigned char* p = (const unsigned char*) bsd_control;
	unsigned char* q = (unsigned char*) linux_control;
	size_t off = 0, out = 0;

	while (bsd_len - off >= BSD_HDR_LEN)
	{
		struct bsd_cmsghdr h;
		struct linux_cmsghdr lh;
		const struct cmsg_map* m = NULL;
		enum lookup_result r;
		size_t dlen;

		memcpy(&h, p + off, sizeof(h));
		dlen = h.cmsg_len - BSD_HDR_LEN;
		r = lookup_bsd(h.cmsg_level, h.cmsg_type, &m);

		if (r == LOOKUP_MAPPED || r == LOOKUP_PASS)
		{
			lh.cmsg_len = LINUX_CMSG_LEN(dlen);
			lh.cmsg_level = (r == LOOKUP_MAPPED) ? m->linux_level : h.cmsg_level;
			lh.cmsg_type = (r == LOOKUP_MAPPED) ? m->linux_type : h.cmsg_type;

			memset(q + out, 0, LINUX_CMSG_SPACE(dlen));
			memcpy(q + out, &lh, sizeof(lh));
			memcpy(q + out + LINUX_HDR_LEN, p + off + BSD_HDR_LEN, dlen);
			out += LINUX_CMSG_SPACE(dlen);
		}

		advance(&off, bsd_len, h.cmsg_len, sizeof(unsigned int));
	}

	return out;
}

size_t cmsg_linux_space_for_bsd(size_t bsd_len)
{
	if (bsd_len < BSD_HDR_LEN)
		return 0;
	if (bsd_len > CMSG_RECV_MAX)
		bsd_len = CMSG_RECV_MAX;

	// A Darwin message of d payload bytes takes at least BSD_CMSG_LEN(d) =
	// 12 + d bytes; its Linux form takes at most LINUX_CMSG_SPACE(4) = 24
	// bytes for d = 1 (an int that becomes a byte) and 16 + d + 7 otherwise.
	// Twice the Darwin size therefore always holds whatever would fit.
	return 2 * bsd_len;
}

static void close_fds(const unsigned char* data, size_t from, size_t count)
{
	for (size_t i = from; i < count; i++)
	{
		int fd;
		memcpy(&fd, data + i * sizeof(int), sizeof(int));
		LINUX_SYSCALL1(__NR_close, fd);
	}
}

static void put_bsd(unsigned char* dst, size_t room, int level, int type,
		const unsigned char* payload, size_t dlen, const struct cmsg_map* m)
{
	struct bsd_cmsghdr h;
	size_t space = BSD_CMSG_SPACE(dlen);

	h.cmsg_len = (unsigned int) BSD_CMSG_LEN(dlen);
	h.cmsg_level = level;
	h.cmsg_type = type;
	memcpy(dst, &h, sizeof(h));

	if (m != NULL && m->kind == KIND_INT_TO_BYTE && dlen == 1)
	{
		int v;
		memcpy(&v, payload, sizeof(v));
		dst[BSD_HDR_LEN] = (unsigned char) v;
	}
	else
		memcpy(dst + BSD_HDR_LEN, payload, dlen);

	// Clear the alignment padding when the buffer has room for it.
	if (space > room)
		space = room;
	if (space > BSD_CMSG_LEN(dlen))
		memset(dst + BSD_CMSG_LEN(dlen), 0, space - BSD_CMSG_LEN(dlen));
}

size_t cmsg_linux_to_bsd(const void* linux_control, size_t linux_len,
		void* bsd_control, size_t bsd_len, int* truncated)
{
	const unsigned char* p = (const unsigned char*) linux_control;
	unsigned char* q = (unsigned char*) bsd_control;
	size_t loff = 0, boff = 0;
	int full = 0;

	while (linux_len - loff >= LINUX_HDR_LEN)
	{
		struct linux_cmsghdr lh;
		const struct cmsg_map* m = NULL;
		const unsigned char* payload;
		enum lookup_result r;
		size_t dlen, out_dlen;
		int level, type;

		memcpy(&lh, p + loff, sizeof(lh));
		if (lh.cmsg_len < LINUX_HDR_LEN || lh.cmsg_len > linux_len - loff)
			break;

		payload = p + loff + LINUX_HDR_LEN;
		dlen = lh.cmsg_len - LINUX_HDR_LEN;
		r = lookup_linux(lh.cmsg_level, lh.cmsg_type, &m);

		if (r == LOOKUP_DROP)
		{
			// Nothing in the drop set carries descriptors, so leaving it out
			// cannot leak any.
			advance(&loff, linux_len, lh.cmsg_len, sizeof(size_t));
			continue;
		}

		level = (r == LOOKUP_MAPPED) ? m->bsd_level : lh.cmsg_level;
		type = (r == LOOKUP_MAPPED) ? m->bsd_type : lh.cmsg_type;
		out_dlen = bsd_payload_out_len(m, dlen);

		// Once something has not fitted, nothing after it is delivered
		// either: the caller sees a prefix of the messages, as on macOS.
		if (!full && bsd_len - boff >= BSD_CMSG_LEN(out_dlen))
		{
			put_bsd(q + boff, bsd_len - boff, level, type, payload, out_dlen, m);
			advance(&boff, bsd_len, BSD_CMSG_LEN(out_dlen), sizeof(unsigned int));
		}
		else
		{
			int first = !full;

			full = 1;
			*truncated = 1;

			if (m != NULL && m->kind == KIND_FDS)
			{
				size_t nfds = dlen / sizeof(int);
				size_t fit = 0;

				// Deliver the descriptors that fit as a shorter SCM_RIGHTS
				// message, and close the others: they are already installed
				// in this process, but the caller will never learn their
				// numbers.
				if (first && bsd_len - boff >= BSD_CMSG_LEN(sizeof(int)))
				{
					fit = (bsd_len - boff - BSD_HDR_LEN) / sizeof(int);
					if (fit > nfds)
						fit = nfds;
					put_bsd(q + boff, bsd_len - boff, level, type, payload, fit * sizeof(int), m);
					advance(&boff, bsd_len, BSD_CMSG_LEN(fit * sizeof(int)), sizeof(unsigned int));
				}

				close_fds(payload, fit, nfds);
			}
		}

		advance(&loff, linux_len, lh.cmsg_len, sizeof(size_t));
	}

	return boff;
}
