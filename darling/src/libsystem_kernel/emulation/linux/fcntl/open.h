#ifndef LINUX_OPEN_H
#define LINUX_OPEN_H

long sys_open(const char* filename, int flags, unsigned int mode);
long sys_open_nocancel(const char* filename, int flags, unsigned int mode);


// Based on <bits/fcntl-linux.h>
#define LINUX_O_ACCMODE 0003
#define LINUX_O_RDONLY 00
#define LINUX_O_WRONLY 01
#define LINUX_O_RDWR 02

#ifndef LINUX_O_CREAT
#define LINUX_O_CREAT 0100
#endif
#ifndef LINUX_O_EXCL
#define LINUX_O_EXCL 0200
#endif
#ifndef LINUX_O_NOCTTY
#define LINUX_O_NOCTTY 0400
#endif
#ifndef LINUX_O_TRUNC
#define LINUX_O_TRUNC 01000
#endif
#ifndef LINUX_O_APPEND
#define LINUX_O_APPEND 02000
#endif
#ifndef LINUX_O_NONBLOCK
#define LINUX_O_NONBLOCK 04000
#endif
#ifndef LINUX_O_NDELAY
#define LINUX_O_NDELAY O_NONBLOCK
#endif
#ifndef LINUX_O_SYNC
#define LINUX_O_SYNC 04010000
#endif
#ifndef LINUX_O_ASYNC
#define LINUX_O_ASYNC 020000
#endif
#ifndef LINUX_O_LARGEFILE
#define LINUX_O_LARGEFILE 0100000
#endif
#ifndef LINUX_O_DIRECTORY
#define LINUX_O_DIRECTORY 0200000
#endif
#ifndef LINUX_O_NOFOLLOW
#define LINUX_O_NOFOLLOW 0400000
#endif
#ifndef LINUX_O_CLOEXEC
#define LINUX_O_CLOEXEC 02000000
#endif
#ifndef LINUX_O_DIRECT
#define LINUX_O_DIRECT 040000
#endif
#ifndef LINUX_O_NOATIME
#define LINUX_O_NOATIME 01000000
#endif
#ifndef LINUX_O_PATH
#define LINUX_O_PATH 010000000
#endif
#ifndef LINUX_O_DSYNC
#define LINUX_O_DSYNC 010000
#endif
#ifndef LINUX_O_TMPFILE
#define LINUX_O_TMPFILE (020000000 | LINUX_O_DIRECTORY)
#endif

#define LINUX_O_FSYNC LINUX_O_SYNC


#define BSD_O_RDONLY 0
#define BSD_O_WRONLY 1
#define BSD_O_RDWR 2
#define BSD_O_NONBLOCK 4
#define BSD_O_APPEND 8
#define BSD_O_SHLOCK 0x10
#define BSD_O_EXLOCK 0x20
#define BSD_O_ASYNC 0x40
#define BSD_O_NOFOLLOW 0x100
#define BSD_O_CREAT 0x200
#define BSD_O_TRUNC 0x400
#define BSD_O_EXCL 0x800
#define BSD_O_NOCTTY 0x20000
#define BSD_O_DIRECTORY 0x100000
#define BSD_O_SYMLINK 0x200000
#define BSD_O_CLOEXEC 0x1000000

int oflags_bsd_to_linux(int flags);

#endif

