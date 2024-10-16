#ifndef DARLING_BSD_EMULATION_FCNTL_H
#define DARLING_BSD_EMULATION_FCNTL_H

long sys_fcntl(int fd, int cmd, long arg);
long sys_fcntl_nocancel(int fd, int cmd, long arg);

// TODO: Move into conversion

enum {
	LINUX_F_DUPFD = 0,
	LINUX_F_GETFD,
	LINUX_F_SETFD,
	LINUX_F_GETFL,
	LINUX_F_SETFL,
	LINUX_F_GETLK,
	LINUX_F_SETLK,
	LINUX_F_SETLKW,
	LINUX_F_SETOWN,
	LINUX_F_GETOWN,
	LINUX_F_SETSIG,
	LINUX_F_GETSIG,
	LINUX_F_GETLK64,
	LINUX_F_SETLK64,
	LINUX_F_SETLKW64,
	LINUX_F_SETOWN_EX,
	LINUX_F_GETOWN_EX,
	LINUX_F_GETOWNER_UIDS,
	LINUX_F_DUPFD_CLOEXEC = 1030,
};

enum {
	BSD_F_DUPFD = 0,
	BSD_F_GETFD,
	BSD_F_SETFD,
	BSD_F_GETFL,
	BSD_F_SETFL,
	BSD_F_GETOWN,
	BSD_F_SETOWN,
	BSD_F_GETLK,
	BSD_F_SETLK,
	BSD_F_SETLKW,
	BSD_F_SETLKWTIMEOUT,
	BSD_F_FLUSH_DATA = 40,
	BSD_F_CHKCLEAN,
	BSD_F_PREALLOCATE,
	BSD_F_SETSIZE,
	BSD_F_RDADVISE,
	BSD_F_RDAHEAD,
	BSD_F_GETPATH = 50,
	BSD_F_FULLFSYNC = 51,
	BSD_F_DUPFD_CLOEXEC = 67,
	BSD_F_CHECK_LV = 98,
};

enum {
	LINUX_F_RDLCK = 0,
	LINUX_F_WRLCK,
	LINUX_F_UNLCK,
};

enum {
	BSD_F_RDLCK = 1,
	BSD_F_UNLCK,
	BSD_F_WRLCK,
};

#ifndef FD_CLOEXEC
#   define FD_CLOEXEC 1
#endif

struct bsd_flock {
	long long l_start;
	long long l_len;
	int l_pid;
	short l_type;
	short l_whence;
};

struct linux_flock {
	short int l_type;
	short int l_whence;
	long int l_start;
	long int l_len;
	int l_pid;
};

#endif // DARLING_BSD_EMULATION_FCNTL_H
