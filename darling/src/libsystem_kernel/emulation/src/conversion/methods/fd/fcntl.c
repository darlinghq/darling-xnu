#include <emulation/conversion/methods/fd/fcntl.h>

short int flock_type_linux_to_bsd(short int linux) {
	switch (linux) {
		case LINUX_F_RDLCK: return BSD_F_RDLCK;
		case LINUX_F_WRLCK: return BSD_F_WRLCK;
		case LINUX_F_UNLCK: return BSD_F_UNLCK;
		default:            return -1;
	}
}

short int flock_type_bsd_to_linux(short int bsd) {
	switch (bsd) {
		case BSD_F_RDLCK: return LINUX_F_RDLCK;
		case BSD_F_WRLCK: return LINUX_F_WRLCK;
		case BSD_F_UNLCK: return LINUX_F_UNLCK;
		default:      return -1;
	}
}

int cmd_bsd_to_linux(int bsd) {
	switch (bsd) {
		case BSD_F_DUPFD:  return LINUX_F_DUPFD;
		case BSD_F_GETFD:  return LINUX_F_GETFD;
		case BSD_F_SETFD:  return LINUX_F_SETFD;
		case BSD_F_GETFL:  return LINUX_F_GETFL;
		case BSD_F_SETFL:  return LINUX_F_SETFL;
		case BSD_F_GETOWN: return LINUX_F_GETOWN;
		case BSD_F_SETOWN: return LINUX_F_SETOWN;
		case BSD_F_SETLK:  return LINUX_F_SETLK;
		case BSD_F_SETLKW: return LINUX_F_SETLKW;
		case BSD_F_GETLK:  return LINUX_F_GETLK;
		case BSD_F_DUPFD_CLOEXEC: return LINUX_F_DUPFD_CLOEXEC;
		default:       return -1;
	}
}

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
