#ifndef DARLING_EMULATION_CONVERSION_LINUX_ASM_GENERIC_FCNTL_H
#define DARLING_EMULATION_CONVERSION_LINUX_ASM_GENERIC_FCNTL_H

#include <emulation/conversion/variables/linux/linux/types.h>

#define LINUX_O_ACCMODE     00000003
#define LINUX_O_RDONLY      00000000
#define LINUX_O_WRONLY      00000001
#define LINUX_O_RDWR        00000002
#ifndef LINUX_O_CREAT
#define LINUX_O_CREAT       00000100
#endif
#ifndef LINUX_O_EXCL
#define LINUX_O_EXCL        00000200
#endif
#ifndef LINUX_O_NOCTTY
#define LINUX_O_NOCTTY      00000400
#endif
#ifndef LINUX_O_TRUNC
#define LINUX_O_TRUNC       00001000
#endif
#ifndef LINUX_O_APPEND
#define LINUX_O_APPEND      00002000
#endif
#ifndef LINUX_O_NONBLOCK
#define LINUX_O_NONBLOCK    00004000
#endif
#ifndef LINUX_O_DSYNC
#define LINUX_O_DSYNC       00010000
#endif
#ifndef LINUX_FASYNC
#define LINUX_FASYNC        00020000
#endif
#ifndef LINUX_O_DIRECT
#define LINUX_O_DIRECT      00040000
#endif
#ifndef LINUX_O_LARGEFILE
#define LINUX_O_LARGEFILE   00100000
#endif
#ifndef LINUX_O_DIRECTORY
#define LINUX_O_DIRECTORY   00200000
#endif
#ifndef LINUX_O_NOFOLLOW
#define LINUX_O_NOFOLLOW	00400000
#endif
#ifndef LINUX_O_NOATIME
#define LINUX_O_NOATIME     01000000
#endif
#ifndef LINUX_O_CLOEXEC
#define LINUX_O_CLOEXEC     02000000
#endif

#ifndef LINUX_O_SYNC
#define LINUX___O_SYNC      04000000
#define LINUX_O_SYNC        (LINUX___O_SYNC|LINUX_O_DSYNC)
#endif

#ifndef LINUX_O_PATH
#define LINUX_O_PATH		010000000
#endif

#define LINUX_F_DUPFD   0
#define LINUX_F_GETFD   1
#define LINUX_F_SETFD   2
#define LINUX_F_GETFL   3
#define LINUX_F_SETFL   4
#ifndef LINUX_F_GETLK   
#define LINUX_F_GETLK   5
#define LINUX_F_SETLK   6
#define LINUX_F_SETLKW  7
#endif
#ifndef LINUX_F_SETOWN
#define LINUX_F_SETOWN	8
#define LINUX_F_GETOWN	9
#endif
#ifndef LINUX_F_SETSIG
#define LINUX_F_SETSIG	10
#define LINUX_F_GETSIG	11
#endif

#if LINUX___BITS_PER_LONG == 32 /* || defined(LINUX___KERNEL__) */
#ifndef LINUX_F_GETLK64
#define LINUX_F_GETLK64   12
#define LINUX_F_SETLK64   13
#define LINUX_F_SETLKW64  14
#endif
#endif // LINUX___BITS_PER_LONG == 32 /* || defined(LINUX___KERNEL__) */

#ifndef LINUX_F_SETOWN_EX
#define LINUX_F_SETOWN_EX  15
#define LINUX_F_GETOWN_EX  16
#endif

#ifndef LINUX_F_GETOWNER_UIDS
#define LINUX_F_GETOWNER_UIDS  17
#endif

#ifndef LINUX_F_RDLCK
#define LINUX_F_RDLCK  0
#define LINUX_F_WRLCK  1
#define LINUX_F_UNLCK  2
#endif

#define LINUX_F_LINUX_SPECIFIC_BASE 1024

#ifndef LINUX_HAVE_ARCH_STRUCT_FLOCK
struct linux_flock {
	short	l_type;
	short	l_whence;
	linux___kernel_off_t	l_start;
	linux___kernel_off_t	l_len;
	linux___kernel_pid_t	l_pid;
#ifdef LINUX___ARCH_FLOCK_EXTRA_SYSID
	LINUX___ARCH_FLOCK_EXTRA_SYSID
#endif
#ifdef LINUX___ARCH_FLOCK_PAD
	LINUX___ARCH_FLOCK_PAD
#endif
};

struct linux_flock64 {
	short  l_type;
	short  l_whence;
	linux___kernel_loff_t l_start;
	linux___kernel_loff_t l_len;
	linux___kernel_pid_t  l_pid;
#ifdef LINUX___ARCH_FLOCK64_PAD
	LINUX___ARCH_FLOCK64_PAD
#endif
};
#endif // LINUX_HAVE_ARCH_STRUCT_FLOCK

#endif // DARLING_EMULATION_CONVERSION_LINUX_ASM_GENERIC_FCNTL_H