#include <darling/emulation/xnu_syscall/bsd/impl/signal/sigreturn.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>

long sys_sigreturn()
{
	// Apple's libc invokes sigreturn directly,
	// but Linux injects a rt_sigreturn() syscall on its own.
	// Hance we must ignore this call.
	return 0;
}
