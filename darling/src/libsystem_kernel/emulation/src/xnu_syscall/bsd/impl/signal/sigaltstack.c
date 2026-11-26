#include <darling/emulation/xnu_syscall/bsd/impl/signal/sigaltstack.h>

#include <stddef.h>
#include <sys/errno.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/linux_premigration/signal/sigexc.h>

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
	struct bsd_stack previous_oss;

	// oss is filled in before the new stack is read (the same structure may be passed for both, and
	// then the old stack is what stays installed).
	if (oss != NULL)
	{
		ret = LINUX_SYSCALL(__NR_sigaltstack, NULL, &loss);
		if (ret < 0)
			return errno_linux_to_bsd(ret);
		previous_oss = *oss;
		sigexc_altstack_app_view(&loss);
		stack_linux_to_bsd(&loss, oss);
	}

	if (ss == NULL)
		return 0;

	// XNU accepts only SS_DISABLE as an input flag. In particular,
	// do not silently discard unknown bits or pass Linux's value 2.
	// QUIRK: when the new stack is rejected with EINVAL because of its flags, oss is still filled in
	// with the current stack; when it is rejected with ENOMEM (stack too small), oss is left alone.
	// POSIX does not say what happens to oss on failure. Evidence: black-box observation on macOS
	// 26.7.3 (x86_64) and 10.14.4 (x86_64 and i386) (tier 2).
	// Test: sigaltstack-flags (the "rejected call" checks), signal-state-report ("too small").
	if (ss->ss_flags & ~BSD_SS_DISABLE)
		return -EINVAL;

	lss.ss_sp = ss->ss_sp;
	lss.ss_flags = (ss->ss_flags & BSD_SS_DISABLE) ? LINUX_SS_DISABLE : 0;
	lss.ss_size = ss->ss_size;

	sigexc_altstack_will_change();

	ret = LINUX_SYSCALL(__NR_sigaltstack, &lss, NULL);
	if (ret < 0)
	{
		if (oss != NULL)
			*oss = previous_oss;
		return errno_linux_to_bsd(ret);
	}
	return 0;
}
