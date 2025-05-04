#include <darling/emulation/linux_premigration/ext/for-libelfloader.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/xnu_syscall/bsd/helper/bsdthread/cancelable.h>
#include <darling/emulation/common/bsdthread/per_thread_wd.h>
#include <darling/emulation/xnu_syscall/bsd/impl/fcntl/open.h>

VISIBLE
long _open_for_libelfloader(const char* path, int flags, unsigned int mode) {
	int linux_flags;
	int wd;
	int ret;

	CANCELATION_POINT();

	linux_flags = oflags_bsd_to_linux(flags);
	wd = oflags_bsd_to_linux(flags);

	if (sizeof(void*) == 4) {
		linux_flags |= LINUX_O_LARGEFILE;
	}

	ret = LINUX_SYSCALL(__NR_openat, wd, path, linux_flags, mode);
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
};

VISIBLE
long _access_for_libelfloader(const char* path, int mode) {
	int ret;
	int wd;

	wd = get_perthread_wd();

	ret = LINUX_SYSCALL(__NR_faccessat, wd, path, mode, 0);

	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
};
