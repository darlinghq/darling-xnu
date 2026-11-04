#include <darling/emulation/xnu_syscall/bsd/impl/fcntl/fcntl.h>
#include <darling/emulation/common/dc_image_snapshot.h>

#include <sys/errno.h>
#include <sys/fcntl.h>
#include <limits.h>
#include <string.h>
#include <sys/stat.h>

#include <darling/emulation/xnu_syscall/bsd/impl/fcntl/open.h>
#include <darling/emulation/common/base.h>
#include <darling/emulation/common/simple.h>
#include <darling/emulation/conversion/fcntl/fcntl.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/xnu_syscall/bsd/helper/bsdthread/cancelable.h>
#include <darling/emulation/xnu_syscall/bsd/helper/misc/fdpath.h>
#include <darling/emulation/xnu_syscall/bsd/helper/misc/nofile_limit.h>
#include <darling/emulation/xnu_syscall/bsd/impl/misc/csops.h>
#include <darling/emulation/xnu_syscall/bsd/impl/stat/fstat.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/close.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/pread.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/write.h>

#include <darlingserver/rpc.h>

#ifndef O_NOFOLLOW
#   define O_NOFOLLOW 0x0100
#endif
#ifndef O_DIRECTORY
#   define O_DIRECTORY 0x100000
#endif
#ifndef MAXPATHLEN
#	define MAXPATHLEN 1024
#endif

int oflags_bsd_to_linux(int flags);
int oflags_linux_to_bsd(int flags);

extern int sprintf(char *str, const char *format, ...);
long sys_readlink(const char* path, char* buf, unsigned long bsize);

extern void kqueue_dup(int oldfd, int newfd);

static short int flock_type_linux_to_bsd(short int linux) {
	switch (linux) {
		case LINUX_F_RDLCK: return F_RDLCK;
		case LINUX_F_WRLCK: return F_WRLCK;
		case LINUX_F_UNLCK: return F_UNLCK;
		default:            return -1;
	};
};

static short int flock_type_bsd_to_linux(short int bsd) {
	switch (bsd) {
		case F_RDLCK: return LINUX_F_RDLCK;
		case F_WRLCK: return LINUX_F_WRLCK;
		case F_UNLCK: return LINUX_F_UNLCK;
		default:      return -1;
	};
};

static int cmd_bsd_to_linux(int bsd) {
	switch (bsd) {
		case F_DUPFD:  return LINUX_F_DUPFD;
		case F_GETFD:  return LINUX_F_GETFD;
		case F_SETFD:  return LINUX_F_SETFD;
		case F_GETFL:  return LINUX_F_GETFL;
		case F_SETFL:  return LINUX_F_SETFL;
		case F_GETOWN: return LINUX_F_GETOWN;
		case F_SETOWN: return LINUX_F_SETOWN;
		case F_SETLK:  return LINUX_F_SETLK;
		case F_SETLKW: return LINUX_F_SETLKW;
		case F_GETLK:  return LINUX_F_GETLK;
		case F_DUPFD_CLOEXEC: return LINUX_F_DUPFD_CLOEXEC;
		default:       return -1;
	};
};

// F_ADDFILESIGS, F_ADDFILESIGS_RETURN: darlingserver reads the signature of
// the Mach-O image at `fs_file_start` itself. Only a policy that enforces
// code signing refuses an image whose signature is not valid; otherwise
// nothing is refused, and an image without a valid signature covers nothing.
static long add_file_signatures(int fd, int cmd, fsignatures_t* signatures)
{
	uint32_t status = 0, flags = 0, page_size_log2 = 0;
	uint64_t code_limit = 0;
	bool library_validation_ok = false;
	off_t end = 0;
	int ret;

	if (signatures == NULL)
		return -EFAULT;

	ret = dserver_rpc_cs_verify_image(fd, (uint64_t)signatures->fs_file_start,
		&status, &flags, &code_limit, &page_size_log2, &library_validation_ok);
	if (ret < 0)
		return errno_linux_to_bsd(ret);

	if (status == dserver_cs_status_valid) {
		// the signed range ends at the code limit, rounded up to a hash page
		uint64_t page_mask = (page_size_log2 > 0 && page_size_log2 < 32) ? (1ull << page_size_log2) - 1 : 0;
		end = (off_t)((code_limit + page_mask) & ~page_mask);
	} else if (cs_own_policy() == dserver_code_signing_policy_enforce) {
		return -EBADEXEC;
	}

	if (cmd == F_ADDFILESIGS_RETURN)
		signatures->fs_file_start = end;

	return 0;
}

// F_CHECK_LV: whether the calling process's library validation accepts the
// image. Library validation refuses nothing unless the policy enforces code
// signing, so only then is darlingserver asked.
static long check_library_validation(int fd, fchecklv_t* check)
{
	static const char message[] = "library validation refused the image: it is neither a platform binary nor signed by the same team";
	uint32_t status = 0, flags = 0, page_size_log2 = 0;
	uint64_t code_limit = 0;
	bool library_validation_ok = false;
	int ret;

	if (check == NULL)
		return -EFAULT;
	if (cs_own_policy() != dserver_code_signing_policy_enforce)
		return 0;

	ret = dserver_rpc_cs_verify_image(fd, (uint64_t)check->lv_file_start,
		&status, &flags, &code_limit, &page_size_log2, &library_validation_ok);
	if (ret < 0)
		return errno_linux_to_bsd(ret);
	if (library_validation_ok)
		return 0;

	if (check->lv_error_message != NULL && check->lv_error_message_size > 0) {
		size_t length = sizeof(message) - 1;
		if (length >= check->lv_error_message_size)
			length = check->lv_error_message_size - 1;
		memcpy(check->lv_error_message, message, length);
		((char*)check->lv_error_message)[length] = '\0';
	}
	return -EPERM;
}

// DC_F_SNAPSHOT_IMAGE (see dc_image_snapshot.h)
static long snapshot_image(int fd)
{
	static const char name[] = "darling-image-snapshot";
	char buffer[16384];
	// sys_fstat fills the legacy (32-bit inode) layout of the fstat system call,
	// which is not the struct stat this file is compiled with; stat64 is unambiguous
	struct stat64 st;
	long snapshot, ret;
	unsigned long long offset = 0;

	ret = sys_fstat64(fd, &st);
	if (ret < 0)
		return ret;
	if ((st.st_mode & S_IFMT) != S_IFREG || st.st_size <= 0)
		return -EINVAL;
	if ((unsigned long long)st.st_size > DC_SNAPSHOT_IMAGE_MAX)
		return -EFBIG;

	snapshot = LINUX_SYSCALL(__NR_memfd_create, name, LINUX_MFD_CLOEXEC | LINUX_MFD_ALLOW_SEALING);
	if (snapshot < 0)
		return errno_linux_to_bsd(snapshot);
	snapshot = nofile_limit_check_new(snapshot);
	if (snapshot < 0)
		return snapshot;

	while (offset < (unsigned long long)st.st_size) {
		unsigned long long left = (unsigned long long)st.st_size - offset;
		long got, written = 0;

		got = sys_pread_nocancel(fd, buffer, left < sizeof(buffer) ? (int)left : (int)sizeof(buffer), (long long)offset);
		if (got == -EINTR)
			continue;
		if (got <= 0) {
			// the file became shorter while it was copied
			ret = (got < 0) ? got : -EIO;
			goto fail;
		}

		while (written < got) {
			long put = sys_write_nocancel(snapshot, buffer + written, (int)(got - written));
			if (put == -EINTR)
				continue;
			if (put <= 0) {
				ret = (put < 0) ? put : -EIO;
				goto fail;
			}
			written += put;
		}

		offset += (unsigned long long)got;
	}

#ifdef __NR_fcntl64
	ret = LINUX_SYSCALL(__NR_fcntl64, snapshot, LINUX_F_ADD_SEALS,
#else
	ret = LINUX_SYSCALL(__NR_fcntl, snapshot, LINUX_F_ADD_SEALS,
#endif
		LINUX_F_SEAL_SEAL | LINUX_F_SEAL_SHRINK | LINUX_F_SEAL_GROW | LINUX_F_SEAL_WRITE);
	if (ret < 0) {
		ret = errno_linux_to_bsd(ret);
		goto fail;
	}

	return snapshot;

fail:
	sys_close_nocancel(snapshot);
	return ret;
}

long sys_fcntl(int fd, int cmd, long arg)
{
	CANCELATION_POINT();
	return sys_fcntl_nocancel(fd, cmd, arg);
}

long sys_fcntl_nocancel(int fd, int cmd, long arg)
{
	int ret, linux_cmd = cmd_bsd_to_linux(cmd);
	long original_arg = arg;

	switch (cmd)
	{
		case F_ADDFILESIGS:
		case F_ADDFILESIGS_RETURN:
			return add_file_signatures(fd, cmd, (fsignatures_t*)arg);
		case F_CHECK_LV:
			return check_library_validation(fd, (fchecklv_t*)arg);
		case DC_F_SNAPSHOT_IMAGE:
			return snapshot_image(fd);
		case F_SETFD:
			if (arg & ~FD_CLOEXEC)
				return -EINVAL;
			break;
		case F_SETFL:
			arg = oflags_bsd_to_linux(arg);
			break;
		case F_DUPFD:
		case F_DUPFD_CLOEXEC:
			// fcntl(2): EINVAL for a minimum beyond the descriptor limit
			if (arg >= 0 && arg <= INT_MAX && nofile_limit_exceeded((int)arg))
				return -EINVAL;
			break;
		case F_GETPATH:
		{
			ret = fdpath(fd, arg, MAXPATHLEN);
			if (ret < 0)
				return errno_linux_to_bsd(ret);

			return 0;
		}
		case F_SETLK:
		case F_SETLKW:
		case F_GETLK: {
			struct bsd_flock* bsd_struct = arg;
			struct linux_flock* linux_struct = __builtin_alloca(sizeof(struct linux_flock));
			linux_struct->l_type = flock_type_bsd_to_linux(bsd_struct->l_type);
			linux_struct->l_whence = bsd_struct->l_whence;
			linux_struct->l_start = bsd_struct->l_start;
			linux_struct->l_len = bsd_struct->l_len;
			linux_struct->l_pid = bsd_struct->l_pid;
			arg = linux_struct;
		} break;
		case F_FULLFSYNC: {
			ret = LINUX_SYSCALL1(__NR_fsync, fd);
			if (ret < 0)
				ret = errno_linux_to_bsd(ret);
			return 0;
		};
		case F_NOCACHE:
		{
			// Only a caching hint; Linux has no per-descriptor equivalent
			// (O_DIRECT has alignment requirements that F_NOCACHE does
			// not), so accept it on any valid descriptor.
#ifdef __NR_fcntl64
			ret = LINUX_SYSCALL(__NR_fcntl64, fd, LINUX_F_GETFD, 0);
#else
			ret = LINUX_SYSCALL(__NR_fcntl, fd, LINUX_F_GETFD, 0);
#endif
			if (ret < 0)
				return errno_linux_to_bsd(ret);
			return 0;
		}
		// Linux files have no per-file data protection, which is iOS's
		// class D. iOS apps (Cydia's setnsfpn) query and set it.
		case F_GETPROTECTIONCLASS:
			return 4;
		case F_SETPROTECTIONCLASS:
			return 0;
		// TODO: implement remaining commands
		default:
			if (linux_cmd == -1)
				return -EINVAL;
	}

#ifdef __NR_fcntl64
	ret = LINUX_SYSCALL(__NR_fcntl64, fd, linux_cmd, arg);
#else
	ret = LINUX_SYSCALL(__NR_fcntl, fd, linux_cmd, arg);
#endif
	if (ret < 0)
	{
		ret = errno_linux_to_bsd(ret);
		return ret;
	}

	switch (cmd)
	{
		case F_GETFL:
			ret = oflags_linux_to_bsd(ret);
			break;
		case F_SETLK:
		case F_SETLKW:
		case F_GETLK: {
			struct bsd_flock* bsd_struct = original_arg;
			struct linux_flock* linux_struct = arg;
			bsd_struct->l_type = flock_type_linux_to_bsd(linux_struct->l_type);
			bsd_struct->l_whence = linux_struct->l_whence;
			bsd_struct->l_start = linux_struct->l_start;
			bsd_struct->l_len = linux_struct->l_len;
			bsd_struct->l_pid = linux_struct->l_pid;
		} break;
		case F_DUPFD:
		case F_DUPFD_CLOEXEC:
			ret = nofile_limit_check_new(ret);
			if (ret >= 0)
				kqueue_dup(fd, ret);
			break;
	}

	return ret;
}
