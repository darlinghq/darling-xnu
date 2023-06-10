#ifndef _COMMON_AT_H
#define _COMMON_AT_H

// Common declarations for the *at family of system calls

#define LINUX_AT_INVALID 		0x1
#define LINUX_AT_FDCWD	-100
#define LINUX_AT_SYMLINK_NOFOLLOW	0x100
#define LINUX_AT_REMOVEDIR			0x200
#define LINUX_AT_SYMLINK_FOLLOW		0x400
#define LINUX_AT_EMPTY_PATH		0x1000

#define BSD_AT_FDCWD	-2
#define BSD_AT_SYMLINK_NOFOLLOW		0x20
#define BSD_AT_REMOVEDIR			0x80
#define BSD_AT_SYMLINK_FOLLOW		0x40
#define BSD_AT_EACCESS			0x10
#define BSD_AT_REMOVEDIR_DATALESS		0x100
#define BSD_AT_REALDEV 			0x200
#define BSD_AT_FDONLY			0x400

int atflags_bsd_to_linux(int flags);

int atfd(int fd);

#endif

