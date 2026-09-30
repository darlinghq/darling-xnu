#ifndef NETWORK_DUCT_H
#define NETWORK_DUCT_H

#include <stddef.h>

/* Linux's struct sockaddr_un is { sa_family_t sun_family; char
 * sun_path[108]; } and Darwin's is { sa_len_t sun_len; sa_family_t
 * sun_family; char sun_path[104]; }. Both put a 2-byte family in the first two
 * bytes and the path at offset 2, so a single buffer can carry either view and
 * only the path capacity differs.
 *
 * sockaddr_fixup_from_bsd() hands its result straight to connect(), bind() and
 * sendto(), so the Linux view has to match the host's struct sockaddr_un byte
 * for byte. The assertions below pin that down so a future edit cannot quietly
 * reintroduce a layout the host kernel would misread. */
#define SOCKADDR_FIXUP_LINUX_PATH_MAX 108
#define SOCKADDR_FIXUP_BSD_PATH_MAX 104

struct sockaddr_fixup
{
	union
	{
		/* Darwin view: struct sockaddr_un. */
		struct
		{
			unsigned char bsd_length;
			unsigned char bsd_family;
			char bsd_sun_path[SOCKADDR_FIXUP_BSD_PATH_MAX];
		};
		/* Linux view: struct sockaddr_un. */
		struct
		{
			unsigned short linux_family;
			char sun_path[SOCKADDR_FIXUP_LINUX_PATH_MAX];
		};
	};
};

_Static_assert(sizeof(((struct sockaddr_fixup*) 0)->bsd_length) == 1, "Darwin sa_len is one byte");
_Static_assert(sizeof(((struct sockaddr_fixup*) 0)->bsd_family) == 1, "Darwin sa_family is one byte");
_Static_assert(offsetof(struct sockaddr_fixup, bsd_length) == 0, "Darwin sa_len must sit at offset 0");
_Static_assert(offsetof(struct sockaddr_fixup, bsd_family) == 1, "Darwin sa_family must sit at offset 1");
_Static_assert(offsetof(struct sockaddr_fixup, bsd_sun_path) == 2, "Darwin sun_path must sit at offset 2");

_Static_assert(sizeof(((struct sockaddr_fixup*) 0)->linux_family) == 2, "Linux sa_family_t is two bytes");
_Static_assert(offsetof(struct sockaddr_fixup, linux_family) == 0, "Linux sa_family must sit at offset 0");
_Static_assert(offsetof(struct sockaddr_fixup, sun_path) == 2, "Linux sun_path must sit at offset 2");
_Static_assert(sizeof(((struct sockaddr_fixup*) 0)->sun_path) == SOCKADDR_FIXUP_LINUX_PATH_MAX,
	"Linux sun_path is 108 bytes");
_Static_assert(sizeof(struct sockaddr_fixup) == SOCKADDR_FIXUP_LINUX_PATH_MAX + 2,
	"sockaddr_fixup must be exactly as large as the host's struct sockaddr_un");

unsigned long sockaddr_fixup_size_from_bsd(const void* bsd_sockaddr, int bsd_sockaddr_len);
int sockaddr_fixup_from_bsd(struct sockaddr_fixup* out, const void* bsd_sockaddr, int bsd_sockaddr_len);
int sockaddr_fixup_from_linux(struct sockaddr_fixup* out, const void* linux_sockaddr, int linux_sockaddr_len);

#endif // NETWORK_DUCT_H
