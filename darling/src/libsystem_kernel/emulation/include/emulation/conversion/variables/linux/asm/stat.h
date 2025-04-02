#ifndef DARLING_EMULATION_CONVERSION_LINUX_ASM_STAT_H
#define DARLING_EMULATION_CONVERSION_LINUX_ASM_STAT_H

#ifdef st_atime
#	undef st_atime
#	undef st_mtime
#	undef st_ctime
#endif

#ifdef __i386__

struct linux_stat /* linux_stat64 */ {
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

#elif defined(__x86_64__)

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

#else
#error "Missing stat struct for arch"

#endif

#endif // DARLING_EMULATION_CONVERSION_LINUX_ASM_STAT_H
