#ifndef DARLING_EMULATION_CONVERSION_BSD_SYS_FCNTL_H
#define DARLING_EMULATION_CONVERSION_BSD_SYS_FCNTL_H

#define BSD_O_RDONLY        0x0000
#define BSD_O_WRONLY        0x0001
#define BSD_O_RDWR          0x0002
// O_ACCMODE

#define BSD_FREAD           0x00000001
#define BSD_FWRITE          0x00000002
#define BSD_O_NONBLOCK      0x00000004
#define BSD_O_APPEND        0x00000008

#define BSD_O_SHLOCK 0x10
#define BSD_O_EXLOCK 0x20
#define BSD_O_ASYNC 0x40
// O_FSYNC
#define BSD_O_NOFOLLOW 0x100
#define BSD_O_CREAT 0x200
#define BSD_O_TRUNC 0x400
#define BSD_O_EXCL 0x800

#define BSD_O_NOCTTY 0x20000

#define BSD_O_DIRECTORY 0x100000
#define BSD_O_SYMLINK 0x200000

#define BSD_O_CLOEXEC 0x1000000

#define BSD_AT_FDCWD        -2

#define BSD_AT_EACCESS              0x10
#define BSD_AT_SYMLINK_NOFOLLOW		0x20
#define BSD_AT_SYMLINK_FOLLOW		0x40
#define BSD_AT_REMOVEDIR			0x80
#define BSD_AT_REMOVEDIR_DATALESS   0x100
#define BSD_AT_REALDEV              0x200
#define BSD_AT_FDONLY               0x400

enum {
	BSD_F_DUPFD         = 0,
	BSD_F_GETFD         = 1,
	BSD_F_SETFD         = 2,
	BSD_F_GETFL         = 3,
	BSD_F_SETFL         = 4,
	BSD_F_GETOWN        = 5,
	BSD_F_SETOWN        = 6,
	BSD_F_GETLK         = 7,
	BSD_F_SETLK         = 8,
	BSD_F_SETLKW        = 9,
	BSD_F_SETLKWTIMEOUT = 10,

	BSD_F_FLUSH_DATA    = 40,
	BSD_F_CHKCLEAN      = 41,
	BSD_F_PREALLOCATE   = 42,
	BSD_F_SETSIZE       = 43,
	BSD_F_RDADVISE      = 44,
	BSD_F_RDAHEAD       = 45,

	BSD_F_GETPATH       = 50,
	BSD_F_FULLFSYNC     = 51,

	BSD_F_DUPFD_CLOEXEC         = 67,

	BSD_F_CHECK_LV              = 98,
};

#define BSD_FD_CLOEXEC 1

enum {
	BSD_F_RDLCK         = 1,
	BSD_F_UNLCK         = 2,
	BSD_F_WRLCK         = 3,
};

struct bsd_flock {
	long long l_start;
	long long l_len;
	int l_pid;
	short l_type;
	short l_whence;
};

#endif // DARLING_EMULATION_CONVERSION_BSD_SYS_FCNTL_H
