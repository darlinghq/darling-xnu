#ifndef DARLING_CONVERSION_FCNTL_OPEN
#define DARLING_CONVERSION_FCNTL_OPEN

#define LINUX_O_RDONLY 00
#define LINUX_O_WRONLY 01
#define LINUX_O_RDWR 02
#define LINUX_O_CREAT 0100
#define LINUX_O_EXCL 0200
#define LINUX_O_NOCTTY 0400
#define LINUX_O_TRUNC 01000
#define LINUX_O_APPEND 02000
#define LINUX_O_NONBLOCK 04000
#define LINUX_O_SYNC 04010000
#define LINUX_O_ASYNC 020000
#define LINUX_O_CLOEXEC 02000000
// O_DIRECTORY/O_NOFOLLOW/O_LARGEFILE differ between x86_64 and aarch64 in the
// Linux kernel uapi headers. aarch64 overrides them in <asm/fcntl.h> while
// x86_64 inherits asm-generic values. Get this wrong and open(path, O_DIRECTORY)
// becomes open(path, O_DIRECT) on aarch64 → EINVAL on regular directories.
#if defined(__aarch64__) || defined(__arm64__)
#  define LINUX_O_DIRECTORY 040000  /* aarch64: 0x4000 */
#  define LINUX_O_NOFOLLOW 0100000  /* aarch64: 0x8000 */
#  define LINUX_O_LARGEFILE 0400000 /* aarch64: 0x20000 */
#else
#  define LINUX_O_LARGEFILE 0100000
#  define LINUX_O_NOFOLLOW 0400000
#  define LINUX_O_DIRECTORY 0200000
#endif

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

#endif // DARLING_CONVERSION_FCNTL_OPEN
