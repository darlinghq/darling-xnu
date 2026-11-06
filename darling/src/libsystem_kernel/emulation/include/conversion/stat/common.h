#ifndef LINUX_STAT_COMMON_H
#define LINUX_STAT_COMMON_H

#ifdef __i386__

#ifdef st_atime
#	undef st_atime
#	undef st_mtime
#	undef st_ctime
#endif

struct linux_stat {
	unsigned long long      st_dev;
	unsigned char   __pad0[4];

	unsigned long   __st_ino;

	unsigned int    st_mode;
	unsigned int    st_nlink;

	unsigned long   st_uid;
	unsigned long   st_gid;

	unsigned long long      st_rdev;
	unsigned char   __pad3[4];

	long long       st_size;
	unsigned long   st_blksize;

	/* Number 512-byte blocks allocated. */
	unsigned long long      st_blocks;

	unsigned long   st_atime;
	unsigned long   st_atime_nsec;

	unsigned long   st_mtime;
	unsigned int    st_mtime_nsec;

	unsigned long   st_ctime;
	unsigned long   st_ctime_nsec;

	unsigned long long      st_ino;
};

#elif defined(__aarch64__) || defined(__arm64__)

/* Linux ARM64 follows the generic ABI; layout is different from x86_64:
 *  - st_mode/uid/gid come right after dev/ino (no extra st_nlink upfront)
 *  - st_blksize is `int` not `long`
 *  - field order matches asm-generic/stat.h
 */
typedef unsigned long long __kernel_ulong_t;
typedef long long __kernel_long_t;

#ifdef st_atime
#	undef st_atime
#	undef st_mtime
#	undef st_ctime
#	undef __unused
#endif

struct linux_stat {
	__kernel_ulong_t  st_dev;          /* 0 */
	__kernel_ulong_t  st_ino;          /* 8 */
	unsigned int      st_mode;         /* 16 */
	unsigned int      st_nlink;        /* 20 */
	unsigned int      st_uid;          /* 24 */
	unsigned int      st_gid;          /* 28 */
	__kernel_ulong_t  st_rdev;         /* 32 */
	__kernel_ulong_t  __pad1;          /* 40 */
	__kernel_long_t   st_size;         /* 48 */
	int               st_blksize;      /* 56 */
	int               __pad2;          /* 60 */
	__kernel_long_t   st_blocks;       /* 64 */

	__kernel_ulong_t  st_atime;        /* 72 */
	__kernel_ulong_t  st_atime_nsec;   /* 80 */
	__kernel_ulong_t  st_mtime;        /* 88 */
	__kernel_ulong_t  st_mtime_nsec;   /* 96 */
	__kernel_ulong_t  st_ctime;        /* 104 */
	__kernel_ulong_t  st_ctime_nsec;   /* 112 */
	unsigned int      __unused4;       /* 120 */
	unsigned int      __unused5;       /* 124 */
};

#else

typedef unsigned long long __kernel_ulong_t;
typedef long long __kernel_long_t;

#ifdef st_atime
#	undef st_atime
#	undef st_mtime
#	undef st_ctime
#	undef __unused
#endif

struct linux_stat {
	__kernel_ulong_t        st_dev;
	__kernel_ulong_t        st_ino;
	__kernel_ulong_t        st_nlink;

	unsigned int            st_mode;
	unsigned int            st_uid;
	unsigned int            st_gid;
	unsigned int            __pad0;
	__kernel_ulong_t        st_rdev;
	__kernel_long_t         st_size;
	__kernel_long_t         st_blksize;
	__kernel_long_t         st_blocks;      /* Number 512-byte blocks allocated. */

	__kernel_ulong_t        st_atime;
	__kernel_ulong_t        st_atime_nsec;
	__kernel_ulong_t        st_mtime;
	__kernel_ulong_t        st_mtime_nsec;
	__kernel_ulong_t        st_ctime;
	__kernel_ulong_t        st_ctime_nsec;
	__kernel_long_t         __unused_var[3];
};

#endif

struct bsd_statfs
{
	short f_otype;
	short f_oflags;
	long f_bsize;
	long f_iosize;
	long f_blocks;
	long f_bfree;
	long f_bavail;
	long f_files;
	long f_ffree;
	unsigned long long f_fsid;
	unsigned int f_owner;
	short f_reserved1;
	short f_type;
	long f_flags;
	long f_reserved2[2];
	char f_fstypename[16];
	char f_mntonname[1024];
	char f_mntfromname[1024];
	char f_reserved3;
	long f_reserved4[4];
};

struct bsd_statfs64
{
	//short f_otype;
	//short f_oflags;
	unsigned int f_bsize;
	int f_iosize;
	long long f_blocks;
	long long f_bfree;
	long long f_bavail;
	long long f_files;
	long long f_ffree;
	unsigned long long f_fsid;
	unsigned int f_owner;
	unsigned int f_type;
	unsigned int f_flags;
	unsigned int f_fssubtype;
	char f_fstypename[16];
	char f_mntonname[1024];
	char f_mntfromname[1024];
	long long f_reserved4[4];
};

struct linux_statfs64
{
	long f_type;
	long f_bsize;
	unsigned long long f_blocks;
	unsigned long long f_bfree;
	unsigned long long f_bavail;
	unsigned long long f_files;
	unsigned long long f_ffree;
	unsigned long long f_fsid;
	long f_namelen;
	long f_frsize;
	long f_flags;
	long f_spare[4];
};

struct stat;
#if !defined(__aarch64__) && !defined(__arm64__)
struct stat64;
#endif

void stat_linux_to_bsd(const struct linux_stat* lstat, struct stat* stat);
#if defined(__aarch64__) || defined(__arm64__)
void stat_linux_to_bsd64(const struct linux_stat* lstat, struct stat* stat);
#else
void stat_linux_to_bsd64(const struct linux_stat* lstat, struct stat64* stat);
#endif
void statfs_linux_to_bsd(const struct linux_statfs64* lstat, struct bsd_statfs* stat);
void statfs_linux_to_bsd64(const struct linux_statfs64* lstat, struct bsd_statfs64* stat);

#endif // LINUX_STAT_COMMON_H
