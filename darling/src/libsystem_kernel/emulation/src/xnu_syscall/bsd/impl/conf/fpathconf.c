#include <darling/emulation/xnu_syscall/bsd/impl/conf/fpathconf.h>

#include <sys/errno.h>
#include <sys/stat.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/xnu_syscall/bsd/impl/stat/fstat.h>
#include <darling/emulation/conversion/stat/common.h>

enum {
	BSD_PC_LINK_MAX = 1,
	BSD_PC_MAX_CANON,
	BSD_PC_MAX_INPUT,
	BSD_PC_NAME_MAX,
	BSD_PC_PATH_MAX,
	BSD_PC_PIPE_BUF,
	BSD_PC_CHOWN_RESTRICTED,
	BSD_PC_NO_TRUNC,
	BSD_PC_VDISABLE,
	BSD_PC_NAME_CHARS_MAX,
	BSD_PC_CASE_SENSITIVE,
	BSD_PC_CASE_PRESERVING,
	BSD_PC_EXTENDED_SECURITY_NP,
	BSD_PC_AUTH_OPAQUE_NP,
	BSD_PC_2_SYMLINKS,
	BSD_PC_ALLOC_SIZE_MIN,
	BSD_PC_ASYNC_IO,
	BSD_PC_FILESIZEBITS,
	BSD_PC_PRIO_IO,
	BSD_PC_REC_INCR_XFER_SIZE,
	BSD_PC_REC_MAX_XFER_SIZE,
	BSD_PC_REC_MIN_XFER_SIZE,
	BSD_PC_REC_XFER_ALIGN,
	BSD_PC_SYMLINK_MAX,
	BSD_PC_SYNC_IO,
	BSD_PC_XATTR_SIZE_BITS,
	BSD_PC_MIN_HOLE_SIZE
};

// _PC_ answers (sys/unistd.h), observed with pathconf() and fpathconf() on macOS 26.7.3 and 10.14.4 (APFS,
// black-box; the two agree except for names 18 and 26 on a regular file or directory, which are 56 on 26.7.3 and
// 64 on 10.14.4: the wider value is used). The answers depend on what the descriptor or path refers to, so there is
// one table per kind. INV marks a name that is EINVAL for that kind. Names 0 and above 27 are EINVAL for every kind.
#define INV (-1)
#define MAX_NAME BSD_PC_MIN_HOLE_SIZE

// Directories and regular files (and symlinks, which pathconf follows).
// Deliberate difference: macOS reports 1 for _PC_EXTENDED_SECURITY_NP, but the guest has no access control lists, so
// the truthful answer is 0 (here and in the FIFO and device tables). The default APFS format is case insensitive and
// case preserving, and the vchroot lookup falls back to case-insensitive matching (vchroot_userspace.c), so the guest
// namespace behaves the same.
static const long values_file[MAX_NAME + 1] = {
	[BSD_PC_LINK_MAX] = 32767, [BSD_PC_MAX_CANON] = INV, [BSD_PC_MAX_INPUT] = INV, [BSD_PC_NAME_MAX] = 255,
	[BSD_PC_PATH_MAX] = 1024, [BSD_PC_PIPE_BUF] = 512, [BSD_PC_CHOWN_RESTRICTED] = 200112,
	[BSD_PC_NO_TRUNC] = 200112, [BSD_PC_VDISABLE] = INV,
	[BSD_PC_NAME_CHARS_MAX] = 255, [BSD_PC_CASE_SENSITIVE] = 0, [BSD_PC_CASE_PRESERVING] = 1,
	[BSD_PC_EXTENDED_SECURITY_NP] = 0, [BSD_PC_AUTH_OPAQUE_NP] = 0, [BSD_PC_2_SYMLINKS] = 1,
	[BSD_PC_ALLOC_SIZE_MIN] = 1, [BSD_PC_ASYNC_IO] = 1, [BSD_PC_FILESIZEBITS] = 64, [BSD_PC_PRIO_IO] = 0,
	[BSD_PC_REC_INCR_XFER_SIZE] = 4096, [BSD_PC_REC_MAX_XFER_SIZE] = 65536, [BSD_PC_REC_MIN_XFER_SIZE] = 4096,
	[BSD_PC_REC_XFER_ALIGN] = 4096, [BSD_PC_SYMLINK_MAX] = 255, [BSD_PC_SYNC_IO] = 0,
	[BSD_PC_XATTR_SIZE_BITS] = 64, [BSD_PC_MIN_HOLE_SIZE] = 4096
};

// Named pipes (FIFOs).
static const long values_fifo[MAX_NAME + 1] = {
	[BSD_PC_LINK_MAX] = 32767, [BSD_PC_MAX_CANON] = INV, [BSD_PC_MAX_INPUT] = INV, [BSD_PC_NAME_MAX] = INV,
	[BSD_PC_PATH_MAX] = INV, [BSD_PC_PIPE_BUF] = 512, [BSD_PC_CHOWN_RESTRICTED] = 200112,
	[BSD_PC_NO_TRUNC] = INV, [BSD_PC_VDISABLE] = INV,
	[BSD_PC_NAME_CHARS_MAX] = INV, [BSD_PC_CASE_SENSITIVE] = INV, [BSD_PC_CASE_PRESERVING] = INV,
	[BSD_PC_EXTENDED_SECURITY_NP] = 0, [BSD_PC_AUTH_OPAQUE_NP] = 0, [BSD_PC_2_SYMLINKS] = 1,
	[BSD_PC_ALLOC_SIZE_MIN] = 1, [BSD_PC_ASYNC_IO] = 1, [BSD_PC_FILESIZEBITS] = INV, [BSD_PC_PRIO_IO] = 0,
	[BSD_PC_REC_INCR_XFER_SIZE] = 4096, [BSD_PC_REC_MAX_XFER_SIZE] = 65536, [BSD_PC_REC_MIN_XFER_SIZE] = 4096,
	[BSD_PC_REC_XFER_ALIGN] = 4096, [BSD_PC_SYMLINK_MAX] = 255, [BSD_PC_SYNC_IO] = 0,
	[BSD_PC_XATTR_SIZE_BITS] = INV, [BSD_PC_MIN_HOLE_SIZE] = INV
};

// Character devices (a terminal and /dev/null answer alike); block devices are treated the same, which is checked
// only for _PC_LINK_MAX and _PC_PATH_MAX (32767 and EINVAL, as for a character device).
static const long values_device[MAX_NAME + 1] = {
	[BSD_PC_LINK_MAX] = 32767, [BSD_PC_MAX_CANON] = 1024, [BSD_PC_MAX_INPUT] = 1024, [BSD_PC_NAME_MAX] = INV,
	[BSD_PC_PATH_MAX] = INV, [BSD_PC_PIPE_BUF] = 512, [BSD_PC_CHOWN_RESTRICTED] = 200112,
	[BSD_PC_NO_TRUNC] = INV, [BSD_PC_VDISABLE] = 255,
	[BSD_PC_NAME_CHARS_MAX] = INV, [BSD_PC_CASE_SENSITIVE] = INV, [BSD_PC_CASE_PRESERVING] = INV,
	[BSD_PC_EXTENDED_SECURITY_NP] = 0, [BSD_PC_AUTH_OPAQUE_NP] = 0, [BSD_PC_2_SYMLINKS] = 1,
	[BSD_PC_ALLOC_SIZE_MIN] = 1, [BSD_PC_ASYNC_IO] = 1, [BSD_PC_FILESIZEBITS] = INV, [BSD_PC_PRIO_IO] = 0,
	[BSD_PC_REC_INCR_XFER_SIZE] = 4096, [BSD_PC_REC_MAX_XFER_SIZE] = 65536, [BSD_PC_REC_MIN_XFER_SIZE] = 4096,
	[BSD_PC_REC_XFER_ALIGN] = 4096, [BSD_PC_SYMLINK_MAX] = 255, [BSD_PC_SYNC_IO] = 0,
	[BSD_PC_XATTR_SIZE_BITS] = 31, [BSD_PC_MIN_HOLE_SIZE] = INV
};

// Anonymous pipes and sockets: only _PC_PIPE_BUF is answered.
static const long values_anonymous[MAX_NAME + 1] = {
	[BSD_PC_PIPE_BUF] = 512,
	[BSD_PC_LINK_MAX] = INV, [BSD_PC_MAX_CANON] = INV, [BSD_PC_MAX_INPUT] = INV, [BSD_PC_NAME_MAX] = INV,
	[BSD_PC_PATH_MAX] = INV, [BSD_PC_CHOWN_RESTRICTED] = INV, [BSD_PC_NO_TRUNC] = INV, [BSD_PC_VDISABLE] = INV,
	[BSD_PC_NAME_CHARS_MAX] = INV, [BSD_PC_CASE_SENSITIVE] = INV, [BSD_PC_CASE_PRESERVING] = INV,
	[BSD_PC_EXTENDED_SECURITY_NP] = INV, [BSD_PC_AUTH_OPAQUE_NP] = INV, [BSD_PC_2_SYMLINKS] = INV,
	[BSD_PC_ALLOC_SIZE_MIN] = INV, [BSD_PC_ASYNC_IO] = INV, [BSD_PC_FILESIZEBITS] = INV, [BSD_PC_PRIO_IO] = INV,
	[BSD_PC_REC_INCR_XFER_SIZE] = INV, [BSD_PC_REC_MAX_XFER_SIZE] = INV, [BSD_PC_REC_MIN_XFER_SIZE] = INV,
	[BSD_PC_REC_XFER_ALIGN] = INV, [BSD_PC_SYMLINK_MAX] = INV, [BSD_PC_SYNC_IO] = INV,
	[BSD_PC_XATTR_SIZE_BITS] = INV, [BSD_PC_MIN_HOLE_SIZE] = INV
};

long pathconf_for_mode(unsigned int mode, int name)
{
	const long* table;

	if (name < 1 || name > MAX_NAME)
		return -EINVAL;
	switch (mode & S_IFMT) {
	case S_IFREG:
	case S_IFDIR:
	case S_IFLNK:
		table = values_file;
		break;
	case S_IFIFO:
		table = values_fifo;
		break;
	case S_IFCHR:
	case S_IFBLK:
		table = values_device;
		break;
	default:
		table = values_anonymous;
		break;
	}
	return table[name] == INV ? -EINVAL : table[name];
}

// Linux reports both ends of an anonymous pipe as a FIFO, like macOS does in fstat; the
// anonymous pipe is told apart from a named FIFO by living on the kernel's pipe filesystem
// (PIPEFS_MAGIC), and gets the anonymous table.
static int fd_is_anonymous_pipe(int fd)
{
	struct linux_statfs64 lbuf;
	int ret;

#ifdef __NR_fstatfs64
	ret = LINUX_SYSCALL3(__NR_fstatfs64, fd, sizeof(lbuf), &lbuf);
#else
	ret = LINUX_SYSCALL2(__NR_fstatfs, fd, &lbuf);
#endif
	return ret >= 0 && (unsigned long)lbuf.f_type == 0x50495045UL;
}

long sys_fpathconf(int fd, int name)
{
	darling_stat64_t st;
	long ret = sys_fstat64(fd, &st);
	unsigned int mode;

	// An invalid descriptor is EBADF whatever the name.
	if (ret < 0)
		return ret;
	mode = st.st_mode;
	if ((mode & S_IFMT) == S_IFIFO && fd_is_anonymous_pipe(fd))
		mode = (mode & ~S_IFMT) | S_IFSOCK;
	return pathconf_for_mode(mode, name);
}
