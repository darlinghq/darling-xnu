#include <darling/emulation/conversion/fcntl/open.h>

#ifndef LINUX_O_PATH
	#define LINUX_O_PATH 0x200000
#endif

int oflags_bsd_to_linux(int flags)
{
	int linux_flags = 0;

	if (flags & BSD_O_RDONLY) /* always false */
		linux_flags |= LINUX_O_RDONLY;
	if (flags & BSD_O_WRONLY)
		linux_flags |= LINUX_O_WRONLY;
	if (flags & BSD_O_RDWR)
		linux_flags |= LINUX_O_RDWR;
	if (flags & BSD_O_NONBLOCK)
		linux_flags |= LINUX_O_NONBLOCK;
	if (flags & BSD_O_APPEND)
		linux_flags |= LINUX_O_APPEND;
	if (flags & BSD_O_CREAT)
		linux_flags |= LINUX_O_CREAT;
	if (flags & BSD_O_TRUNC)
		linux_flags |= LINUX_O_TRUNC;
	if (flags & BSD_O_EXCL)
		linux_flags |= LINUX_O_EXCL;
	if (flags & BSD_O_CLOEXEC)
		linux_flags |= LINUX_O_CLOEXEC;
	if (flags & BSD_O_NOFOLLOW || flags & BSD_O_SYMLINK)
		linux_flags |= LINUX_O_NOFOLLOW;
	if (flags & BSD_O_DIRECTORY)
		linux_flags |= LINUX_O_DIRECTORY;
	if (flags & BSD_O_CLOEXEC)
		linux_flags |= LINUX_O_CLOEXEC;
	if (flags & BSD_O_NOCTTY)
		linux_flags |= LINUX_O_NOCTTY;
	if (flags & BSD_O_SYMLINK)
		linux_flags |= LINUX_O_PATH;

	return linux_flags;
}

int oflags_linux_to_bsd(int flags)
{
	int bsd_flags = 0;

	if (flags & LINUX_O_RDONLY) /* always false */
		bsd_flags |= BSD_O_RDONLY;
	if (flags & LINUX_O_WRONLY)
		bsd_flags |= BSD_O_WRONLY;
	if (flags & LINUX_O_RDWR)
		bsd_flags |= BSD_O_RDWR;
	if (flags & LINUX_O_NONBLOCK)
		bsd_flags |= BSD_O_NONBLOCK;
	if (flags & LINUX_O_APPEND)
		bsd_flags |= BSD_O_APPEND;
	if (flags & LINUX_O_CREAT)
		bsd_flags |= BSD_O_CREAT;
	if (flags & LINUX_O_TRUNC)
		bsd_flags |= BSD_O_TRUNC;
	if (flags & LINUX_O_EXCL)
		bsd_flags |= BSD_O_EXCL;
	if (flags & LINUX_O_CLOEXEC)
		bsd_flags |= BSD_O_CLOEXEC;
	if (flags & LINUX_O_NOFOLLOW)
		bsd_flags |= BSD_O_NOFOLLOW;
	if (flags & LINUX_O_DIRECTORY)
		bsd_flags |= BSD_O_DIRECTORY;
	if (flags & LINUX_O_CLOEXEC)
		bsd_flags |= BSD_O_CLOEXEC;
	if (flags & LINUX_O_NOCTTY)
		bsd_flags |= BSD_O_NOCTTY;

	return bsd_flags;
}
