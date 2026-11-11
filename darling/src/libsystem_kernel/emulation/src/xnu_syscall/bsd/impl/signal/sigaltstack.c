#include <darling/emulation/xnu_syscall/bsd/impl/signal/sigaltstack.h>

#include <stddef.h>
#include <sys/errno.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

#define BSD_SS_ONSTACK 1
#define BSD_SS_DISABLE 4
#define LINUX_SS_ONSTACK 1
#define LINUX_SS_DISABLE 2

static void stack_linux_to_bsd(const struct linux_stack* loss, struct bsd_stack* oss)
{
	oss->ss_sp = loss->ss_sp;
	oss->ss_flags = 0;
	if (loss->ss_flags & LINUX_SS_ONSTACK)
		oss->ss_flags |= BSD_SS_ONSTACK;
	if (loss->ss_flags & LINUX_SS_DISABLE)
		oss->ss_flags |= BSD_SS_DISABLE;
	oss->ss_size = loss->ss_size;
}

long sys_sigaltstack(const struct bsd_stack* ss, struct bsd_stack* oss)
{
	int ret;
	struct linux_stack lss, loss;

	if (ss != NULL)
	{
		// XNU accepts only SS_DISABLE as an input flag. In particular,
		// do not silently discard unknown bits or pass Linux's value 2.
		if (ss->ss_flags & ~BSD_SS_DISABLE)
		{
			// QUIRK: when the new stack is rejected with EINVAL, oss is
			// still filled in with the current stack. POSIX does not say
			// what happens to oss on failure. Evidence: black-box
			// observation on macOS 27.0.1, arm64 and x86_64 (tier 2).
			// Test: sigaltstack-flags (the "rejected call" checks).
			if (oss != NULL)
			{
				ret = LINUX_SYSCALL(__NR_sigaltstack, NULL, &loss);
				if (ret < 0)
					return errno_linux_to_bsd(ret);
				stack_linux_to_bsd(&loss, oss);
			}
			return -EINVAL;
		}
		lss.ss_sp = ss->ss_sp;
		lss.ss_flags = (ss->ss_flags & BSD_SS_DISABLE) ? LINUX_SS_DISABLE : 0;
		lss.ss_size = ss->ss_size;
	}

	ret = LINUX_SYSCALL(__NR_sigaltstack, (ss != NULL) ? &lss : NULL, &loss);
	if (ret < 0)
		return errno_linux_to_bsd(ret);

	if (oss != NULL)
		stack_linux_to_bsd(&loss, oss);
	return 0;
}
