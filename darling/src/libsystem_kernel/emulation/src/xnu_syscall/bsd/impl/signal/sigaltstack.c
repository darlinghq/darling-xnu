#include <emulation/xnu_syscall/bsd/impl/signal/sigaltstack.h>

#include <stddef.h>

#include <emulation/conversion/variables/linux/asm/signal.h>
#include <emulation/linux_api/linux_syscall.h>

long sys_sigaltstack(const struct bsd_sigaltstack* ss, struct bsd_sigaltstack* oss)
{
	int ret;
	struct linux_sigaltstack lss, loss;

	if (ss != NULL)
	{
		lss.ss_sp = ss->ss_sp;
		lss.ss_flags = ss->ss_flags;
		lss.ss_size = ss->ss_size;
	}

	ret = LINUX_SYSCALL(__NR_sigaltstack, (ss != NULL) ? &lss : NULL, &loss);
	if (ret < 0)
		return errno_linux_to_bsd(ret);

	if (oss != NULL)
	{
		oss->ss_sp = loss.ss_sp;
		oss->ss_flags = loss.ss_flags;
		oss->ss_size = loss.ss_size;
	}
	return 0;
}

