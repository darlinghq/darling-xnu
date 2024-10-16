#include "sigexc.h"

#include <stdbool.h>

#include <darlingserver/rpc.h>

#include <emulation/common/simple.h>
#include <emulation/conversion/signal.h>
#include <emulation/conversion/signal/sigaction.h>
#include <emulation/conversion/signal/sigexc.h>
#include <emulation/linux_api/linux_syscall.h>
#include <emulation/xnu_syscall/bsd/impl/signal/kill.h>

#define DEBUG_SIGEXC
#ifdef DEBUG_SIGEXC
#define kern_printf(...) __simple_kprintf(__VA_ARGS__)
#else
#define kern_printf(...)
#endif

extern void linux_sig_restorer(void);

void sigexc_setup1(void);
void sigexc_setup2(void);

void sigexc_setup(void)
{
	darling_sigexc_self();
	sigexc_setup1();
	sigexc_setup2();

#ifdef VARIANT_DYLD
	bool started_suspended;
	int code = dserver_rpc_started_suspended(&started_suspended);
	if (code < 0) {
		__simple_printf("Failed to get started_suspended status: %d\n", code);
		__simple_abort();
	}
	if (started_suspended) {
		kern_printf("sigexc: start_suspended -> suspending (ret to %p)\n", __builtin_return_address(0));
		task_suspend(mach_task_self());
		kern_printf("sigexc: start_suspended -> wokenup (ret to %p)\n", __builtin_return_address(0));
	} else {
		int32_t tracer;
		code = dserver_rpc_get_tracer(&tracer);
		if (code < 0) {
			__simple_printf("Failed to get tracer status: %d\n", code);
			__simple_abort();
		}
		if (tracer != 0) {
			kern_printf("sigexc: already traced -> SIGTRAP\n");
			sys_kill(0, SIGTRAP, 0);
		}
	}
#else

#endif
}

static volatile bool am_i_ptraced = false;

void darling_sigexc_self(void)
{
	am_i_ptraced = true;

	__simple_kprintf("darling_sigexc_self()\n");
	// Make sigexc_handler the handler for all signals in the process
	for (int i = 1; i <= 31; i++)
	{
		if (i == LINUX_SIGSTOP || i == LINUX_SIGKILL || i == LINUX_SIGCHLD)
			continue;

		struct linux_sigaction sa;
		sa.sa_sigaction = (linux_sig_handler*) sigexc_handler;
		sa.sa_mask = 0x7fffffff; // all other standard Unix signals should be blocked while the handler is run
		sa.sa_mask |= (1ull << (SIGNAL_SIGEXC_SUSPEND-1));
		sa.sa_flags = LINUX_SA_RESTORER | LINUX_SA_SIGINFO | LINUX_SA_RESTART | LINUX_SA_ONSTACK;
		sa.sa_restorer = linux_sig_restorer;

		LINUX_SYSCALL(__NR_rt_sigaction, i,
				&sa, NULL,
				sizeof(sa.sa_mask));
	}

#if SIGALTSTACK_GUARD
	sys_mprotect(sigexc_altstack, 4096, PROT_NONE);
#endif

	struct bsd_stack newstack = {
#if SIGALTSTACK_GUARD
		.ss_sp = sigexc_altstack + 4096,
#else
		.ss_sp = sigexc_altstack,
#endif
		.ss_size = SIGEXC_STACK_SIZE,
		.ss_flags = 0
	};
	sys_sigaltstack(&newstack, NULL);
}

void sigexc_setup1(void)
{
	handle_rt_signal(SIGNAL_SIGEXC_SUSPEND);
	handle_rt_signal(SIGNAL_S2C);
}

void sigexc_setup2(void)
{
	linux_sigset_t set;
	set = (1ull << (SIGNAL_SIGEXC_SUSPEND-1));
	set |= (1ull << (SIGNAL_S2C-1));

	LINUX_SYSCALL(__NR_rt_sigprocmask, 1 /* LINUX_SIG_UNBLOCK */,
		&set, NULL, sizeof(linux_sigset_t));
}
