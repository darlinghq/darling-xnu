#include <darling/emulation/xnu_syscall/bsd/impl/stat/fstat.h>

#include <darling/emulation/conversion/stat/common.h>
#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/xnu_syscall/bsd/helper/bpf/bpf_device.h>

// An open /dev/bpfN is a packet socket underneath; report the character
// device it stands for.
static void bpf_fstat_fixup(int fd, struct linux_stat* lstat)
{
	unsigned int bpf_mode;
	int bpf_rdev;

	if (bpf_device_fd_count && bpf_device_stat(fd, &bpf_mode, &bpf_rdev)) {
		lstat->st_mode = bpf_mode;
		lstat->st_rdev = bpf_rdev;
		lstat->st_size = 0;
	}
}

long sys_fstat(int fd, struct stat* stat)
{
	int ret;
	struct linux_stat lstat;

#ifdef __NR_fstat64
	ret = LINUX_SYSCALL(__NR_fstat64, fd, &lstat);
#else
	ret = LINUX_SYSCALL(__NR_fstat, fd, &lstat);
#endif

	if (ret < 0)
		return errno_linux_to_bsd(ret);

	bpf_fstat_fixup(fd, &lstat);
	stat_linux_to_bsd(&lstat, stat);

	return 0;
}

long sys_fstat64(int fd, darling_stat64_t* stat)
{
	int ret;
	struct linux_stat lstat;

#ifdef __NR_fstat64
	ret = LINUX_SYSCALL(__NR_fstat64, fd, &lstat);
#else
	ret = LINUX_SYSCALL(__NR_fstat, fd, &lstat);
#endif

	if (ret < 0)
		return errno_linux_to_bsd(ret);

	bpf_fstat_fixup(fd, &lstat);
	stat_linux_to_bsd64(&lstat, stat);

	return 0;
}
