#include <darling/emulation/xnu_syscall/bsd/impl/process/waitid.h>

#include <stddef.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/conversion/process/wait4.h>
#include <darling/emulation/conversion/signal/duct_signals.h>
#include <darling/emulation/xnu_syscall/bsd/impl/process/wait4.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

static void siginfo_linux_to_bsd(const linux_siginfo_t* linux_si, siginfo_t* si);

long sys_waitid(int idtype, int id, siginfo_t* si, int options)
{
	int ret, linux_options;

	linux_options = waitopts_bsd_to_linux(options);

	linux_siginfo_t linux_si;
	ret = LINUX_SYSCALL(__NR_waitid, idtype, id, &linux_si, linux_options);
	if (ret < 0)
		return errno_linux_to_bsd(ret);

	if (si != NULL)
	{
		siginfo_linux_to_bsd(&linux_si, si);
	}

	return ret;
}

static void siginfo_linux_to_bsd(const linux_siginfo_t* linux_si, siginfo_t* si)
{
	si->si_signo = signum_linux_to_bsd(linux_si->si_signo);
	si->si_errno = errno_linux_to_bsd(linux_si->si_errno);
	si->si_code = linux_si->si_code;
	si->si_pid = linux_si->linux_si_pid;
	si->si_uid = linux_si->linux_si_uid;
	si->si_status = linux_si->linux_si_status;
	si->si_addr = linux_si->linux_si_addr;
	si->si_value = linux_si->linux_si_value;
	si->si_band = linux_si->linux_si_band;
}
