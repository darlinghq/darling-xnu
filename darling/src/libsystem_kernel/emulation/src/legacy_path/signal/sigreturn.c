#include <darling/emulation/legacy_path/signal/sigreturn.h>

#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/errno.h>

long sys_sigreturn()
{
	// Apple's libc invokes sigreturn directly,
	// but Linux injects a rt_sigreturn() syscall on its own.
	// Hance we must ignore this call.
	return 0;
}

