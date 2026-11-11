#include <darling/emulation/xnu_syscall/bsd/helper/network/duct.h>

#include <sys/socket.h>

#include <darling/emulation/conversion/network/duct.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/vchroot_expand.h>
#include <darling/emulation/common/bsdthread/per_thread_wd.h>

#include <stddef.h>

unsigned long sockaddr_fixup_size_from_bsd(const void* bsd_sockaddr, int bsd_sockaddr_len) {
	unsigned long size = bsd_sockaddr_len;

	/* A PF_LOCAL sockaddr is rewritten in place, so the buffer has to hold the
	 * host's struct sockaddr_un on top of whatever the guest passed, and
	 * sockaddr_fixup_from_bsd() copies all bsd_sockaddr_len bytes into it. The
	 * callers accept a socklen of up to 512, so without this floor a guest
	 * asking for a long AF_LOCAL address would write past the allocation. */
	if (((const struct sockaddr_fixup*) bsd_sockaddr)->bsd_family == PF_LOCAL && size < sizeof(struct sockaddr_fixup))
		size = sizeof(struct sockaddr_fixup);

	return size;
}

/* Copies a path out of a fixed-up sockaddr into a NUL-terminated buffer, reading
 * no further than the address actually covers. The guest need not NUL-terminate
 * sun_path within socklen (Darwin's SUN_LEN is offsetof(sun_path) + strlen,
 * with no terminator) and the tail of the buffer past that is uninitialised
 * stack, so a plain strcpy() both overruns the source and turns a short address
 * into a garbage path. */
static void copy_path(char* dest, __SIZE_TYPE__ dest_size, const struct sockaddr_fixup* in, int in_len) {
	const int path_off = (int) offsetof(struct sockaddr_fixup, sun_path);
	__SIZE_TYPE__ max;

	if (in_len <= path_off) {
		/* An address that does not even reach the path carries no path. */
		dest[0] = '\0';
		return;
	}

	/* Never read past sun_path itself, whatever the destination allows. */
	max = sizeof(in->sun_path);
	if (max > dest_size - 1)
		max = dest_size - 1;
	if (in_len < (int) sizeof(*in) && (in_len - path_off) < (int) max)
		max = in_len - path_off;

	strncpy(dest, in->sun_path, max);
	dest[max] = '\0';
}

int sockaddr_fixup_from_bsd(struct sockaddr_fixup* out, const void* bsd_sockaddr, int bsd_sockaddr_len) {
	int ret = bsd_sockaddr_len;

	memcpy(out, bsd_sockaddr, bsd_sockaddr_len);

	out->linux_family = sfamily_bsd_to_linux(out->bsd_family);

	if (out->linux_family == LINUX_PF_LOCAL) {
		struct vchroot_expand_args vc;
		vc.flags = VCHROOT_FOLLOW;
		vc.dfd = get_perthread_wd();

		copy_path(vc.path, sizeof(vc.path), out, bsd_sockaddr_len);

		ret = vchroot_expand(&vc);
		if (ret < 0)
			return errno_linux_to_bsd(ret);

		strncpy(out->sun_path, vc.path, sizeof(out->sun_path) - 1);
		out->sun_path[sizeof(out->sun_path) - 1] = '\0';
		/* + 1 for the NUL terminator: a sockaddr length is the offset of the
		 * path plus the path plus its terminator, and Linux's connect() reads
		 * exactly that many bytes, so omitting it truncates the path by one and
		 * the call fails on a socket that exists. The reverse direction,
		 * sockaddr_fixup_from_linux, has always had the + 1; this one did not. */
		ret = (int) (offsetof(struct sockaddr_fixup, sun_path) + strlen(out->sun_path) + 1);
	}

	return ret;
}

int sockaddr_fixup_from_linux(struct sockaddr_fixup* out, const void* linux_sockaddr, int linux_sockaddr_len, int out_capacity) {
	struct sockaddr_fixup bsd;
	int in_len, ret;

	if (out_capacity < 0)
		out_capacity = 0;

	/* The kernel reports the full address length but stores no more than
	 * the caller's buffer holds, so only that much of the input is there. */
	in_len = linux_sockaddr_len < out_capacity ? linux_sockaddr_len : out_capacity;
	ret = linux_sockaddr_len;

	if (out != linux_sockaddr)
		memmove(out, linux_sockaddr, in_len);
	if (in_len < (int) offsetof(struct sockaddr_fixup, sun_path))
		return ret;

	out->bsd_family = sfamily_linux_to_bsd(out->linux_family);

	// An unnamed AF_UNIX address (such as the peer of an accepted
	// connection) ends after the family, and an abstract one starts with
	// a NUL. Neither has a path to translate, and sun_path holds whatever
	// the caller's buffer contained.
	if (out->bsd_family == PF_LOCAL &&
			(in_len <= (int) __builtin_offsetof(struct sockaddr_fixup, sun_path) ||
			 out->sun_path[0] == '\0')) {
		out->bsd_length = ret;
		return ret;
	}

	if (out->bsd_family == PF_LOCAL) {
		struct vchroot_unexpand_args vc;

		copy_path(vc.path, sizeof(vc.path), out, in_len);

		ret = vchroot_unexpand(&vc);
		if (ret < 0)
			return errno_linux_to_bsd(ret);

		/* Build the Darwin struct sockaddr_un aside and copy out only what
		 * the caller's buffer holds: the translated path can be longer than
		 * the host one, and the buffer can be shorter than a full
		 * sockaddr_un. As on macOS, the returned length is still the full
		 * address length. */
		bsd.bsd_family = PF_LOCAL;
		strncpy(bsd.bsd_sun_path, vc.path, sizeof(bsd.bsd_sun_path) - 1);
		bsd.bsd_sun_path[sizeof(bsd.bsd_sun_path) - 1] = '\0';
		ret = (int) (offsetof(struct sockaddr_fixup, bsd_sun_path) + strlen(bsd.bsd_sun_path) + 1);
		bsd.bsd_length = ret;

		memcpy(out, &bsd, ret < out_capacity ? ret : out_capacity);
		return ret;
	}

	out->bsd_length = ret;
	return ret;
}
