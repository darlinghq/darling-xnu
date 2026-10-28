#include <darling/emulation/xnu_syscall/bsd/impl/misc/fsctl.h>

#include <sys/errno.h>
#include <stddef.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/common/simple.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/access.h>
#include <darling/emulation/xnu_syscall/bsd/impl/stat/fstat.h>
#include <darling/emulation/xnu_syscall/bsd/helper/misc/bsdflags.h>
#include <darling/emulation/linux_premigration/vchroot_expand.h>
#include <darling/emulation/common/bsdthread/per_thread_wd.h>

/*
 * fsctl (242) and ffsctl (245) were missing (ENOSYS). XNU passes requests to
 * the file system (VNOP_IOCTL), and unknown requests fail with ENOTTY. Answer
 * the APFS requests used by the iOS simulator for a plain directory tree:
 * - APFSIOC_GET_PARENT_SYNCROOT: id of the enclosing File Provider sync root,
 *   or 0 when there is none. FileProvider treats an error here as fatal.
 * - APFSIOC_MAINTAIN_DIR_STATS: accepted and ignored.
 * - FSIOC_CAS_BSDFLAGS: compare-and-swap of the chflags flags held in a Linux
 *   extended attribute; the actual flags are returned either way.
 */
#define FSIOC_CAS_BSDFLAGS 0xc00c4114ul
struct fsioc_cas_bsdflags { uint32_t expected_flags, new_flags, actual_flags; };
#define APFSIOC_GET_PARENT_SYNCROOT 0x40084a4aul
#define APFSIOC_MAINTAIN_DIR_STATS  0x80084a02ul

static long fsctl_common(unsigned long cmd, void* data, const char* lpath)
{
	switch (cmd & 0xffffffffu)
	{
		case APFSIOC_MAINTAIN_DIR_STATS:
			return 0;
		case FSIOC_CAS_BSDFLAGS:
		{
			struct fsioc_cas_bsdflags* cas = data;
			if (!cas)
				return -EFAULT;
			uint32_t cur = darling_bsdflags_get(lpath, 0);
			if (cur == cas->expected_flags && cur != cas->new_flags)
			{
				long rv = darling_bsdflags_set(lpath, cas->new_flags);
				if (rv < 0)
					return rv;
			}
			cas->actual_flags = cur;
			return 0;
		}
		case APFSIOC_GET_PARENT_SYNCROOT:
			if (!data)
				return -EFAULT;
			*((uint64_t*)data) = 0;
			return 0;
		default:
			return -ENOTTY;
	}
}

long sys_fsctl(const char* path, unsigned long cmd, void* data, unsigned int options)
{
	long rv = sys_access(path, 0 /* F_OK */);
	if (rv < 0)
		return rv;
	struct vchroot_expand_args vc;
	vc.flags = VCHROOT_FOLLOW;
	vc.dfd = get_perthread_wd();
	strcpy(vc.path, path);
	rv = vchroot_expand(&vc);
	if (rv < 0)
		return errno_linux_to_bsd(rv);
	char buf[1100];
	const char* p = vc.path;
	if (vc.path[0] != '/' && vc.dfd != -100)
	{
		__simple_snprintf(buf, sizeof(buf), "/proc/self/fd/%d/%s", vc.dfd, vc.path);
		p = buf;
	}
	return fsctl_common(cmd, data, p);
}

long sys_ffsctl(int fd, unsigned long cmd, void* data, unsigned int options)
{
	uint64_t st[32];   // a Darwin struct stat64 (144 bytes); only whether fd is valid matters
	long rv = sys_fstat64(fd, (struct stat64*) st);
	if (rv < 0)
		return rv;
	char buf[32];
	return fsctl_common(cmd, data, darling_bsdflags_fd_path(fd, buf));
}
