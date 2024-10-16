#include "fdpath.h"

#include <emulation/darling/server/vchroot_userspace.h>

int fdpath(int fd, char* buf, size_t bufsiz)
{
	struct vchroot_fdpath_args args = {
		.fd = fd,
		.path = buf,
		.maxlen = bufsiz
	};

	return vchroot_fdpath(&args);
}

