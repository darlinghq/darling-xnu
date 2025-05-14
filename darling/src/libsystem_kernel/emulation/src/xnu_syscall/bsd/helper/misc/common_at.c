#include <darling/emulation/conversion/common_at.h>
#include <darling/emulation/common/bsdthread/per_thread_wd.h>

int atflags_bsd_to_linux(int flags)
{
	int linux_flags = 0;

	if (flags & BSD_AT_SYMLINK_NOFOLLOW)
		linux_flags |= LINUX_AT_SYMLINK_NOFOLLOW;
	if (flags & BSD_AT_REMOVEDIR || flags & BSD_AT_REMOVEDIR_DATALESS)
		linux_flags |= LINUX_AT_REMOVEDIR;
	if (flags & BSD_AT_SYMLINK_FOLLOW)
		linux_flags |= LINUX_AT_SYMLINK_FOLLOW;
	if (flags & BSD_AT_FDONLY)
		linux_flags |= LINUX_AT_EMPTY_PATH;
	flags &= ~BSD_AT_SYMLINK_NOFOLLOW;
	flags &= ~BSD_AT_REMOVEDIR;
	flags &= ~BSD_AT_REMOVEDIR_DATALESS;
	flags &= ~BSD_AT_SYMLINK_FOLLOW;
	flags &= ~BSD_AT_EACCESS;
	flags &= ~BSD_AT_REALDEV;
	flags &= ~BSD_AT_FDONLY;
	if (flags != 0)
		linux_flags = LINUX_AT_INVALID;

	return linux_flags;
}

int atfd(int fd)
{
	if (fd == BSD_AT_FDCWD)
		return get_perthread_wd();
	return fd;
}
