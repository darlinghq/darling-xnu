#include "fdpath.h"
#include "mach/lkm.h"
#include "vchroot_expand.h"

int fdpath(int fd, char* buf, size_t bufsiz)
{
	struct vchroot_fdpath_args args = {
		.fd = fd,
		.path = buf,
		.maxlen = bufsiz
	};

	// return lkm_call(NR_vchroot_fdpath, &args);
	return vchroot_fdpath(&args);
}

