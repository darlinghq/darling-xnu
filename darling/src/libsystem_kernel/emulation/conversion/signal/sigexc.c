#include "sigexc.h"

#include <stddef.h>
#include <sys/mman.h>
#include <sys/signal.h>

// #include "../mach/lkm.h"
#include <_libkernel_init.h>

#include <darlingserver/rpc.h>

#include <emulation/conversion/signal/sigaction.h>

#include <emulation/common/simple.h>
#include <emulation/linux_api/linux_syscall.h>
#include <emulation/xnu_syscall/bsd/impl/mman/mman.h>
#include <emulation/xnu_syscall/bsd/impl/signal/sigaction.h>
#include <emulation/xnu_syscall/bsd/impl/signal/sigaltstack.h>
#include <emulation/xnu_syscall/bsd/impl/unistd/exit.h>

// Support for Darwin debugging.
// Unlike other Unix-like systems, macOS doesn't use wait() to handle events in the debugged process.
// wait() only receives termination events.

static void sigexc_fake_suspend(struct linux_ucontext* ctxt);
extern void linux_sig_restorer(void);
extern int getpid(void);

extern _libkernel_functions_t _libkernel_functions;

void darling_sigexc_uninstall(void);
void sigrt_handler(int signum, struct linux_siginfo* info, struct linux_ucontext* ctxt);

#ifndef SIGALTSTACK_GUARD
	#define SIGALTSTACK_GUARD 1
#endif

#if SIGALTSTACK_GUARD
// align it on a page boundary so mprotect works properly
char sigexc_altstack[SIGEXC_STACK_SIZE + 4096ULL] __attribute__((aligned(4096)));
#else
char sigexc_altstack[SIGEXC_STACK_SIZE];
#endif
size_t default_sigaltstack_size = SIGEXC_STACK_SIZE;

static void state_from_kernel(struct linux_ucontext* ctxt, const void* tstate, const void* fstate);
static void state_to_kernel(struct linux_ucontext* ctxt, void* tstate, void* fstate);

#define DEBUG_SIGEXC
#ifdef DEBUG_SIGEXC
#define kern_printf(...) __simple_kprintf(__VA_ARGS__)
#else
#define kern_printf(...)
#endif

#define DUMPREG(regname) kern_printf("sigexc:   " #regname ": 0x%llx\n", regs->regname);

static void dump_gregs(const struct linux_gregset* regs)
{
	unsigned long long* p = (unsigned long long*) regs;
	for (int i = 0; i < 23; i++)
	{
		kern_printf("sigexc:   gregs 0x%llx\n", p[i]);
	}
}

void handle_rt_signal(int signum)
{
	int rv;
	struct linux_sigaction sa;

	sa.sa_sigaction = (linux_sig_handler*)sigrt_handler;
	sa.sa_mask = (1ull << (SIGNAL_SIGEXC_SUSPEND-1)) | (1ull << (signum-1));
	sa.sa_flags = LINUX_SA_RESTORER | LINUX_SA_SIGINFO | LINUX_SA_RESTART | LINUX_SA_ONSTACK;
	sa.sa_restorer = linux_sig_restorer;

	rv = LINUX_SYSCALL(__NR_rt_sigaction, signum,
			&sa, NULL,
			sizeof(sa.sa_mask));

	//char buf[128];
	//__simple_sprintf(xyzbuf, "Setting handler for RT signal %d: %d", signum, rv);
	//external/lkm_call(0x1028, buf);
}

void sigrt_handler(int signum, struct linux_siginfo* info, struct linux_ucontext* ctxt)
{
	int status = dserver_rpc_interrupt_enter();

	if (status != 0) {
		__simple_printf("*** dserver_rpc_interrupt_enter failed with code %d ***\n", status);
		__simple_abort();
	}

	if (signum == SIGNAL_SIGEXC_SUSPEND) {
#if defined(__x86_64__)
	x86_thread_state64_t tstate;
	x86_float_state64_t fstate;
#elif defined(__i386__)
	x86_thread_state32_t tstate;
	x86_float_state32_t fstate;
#endif

	kern_printf("sigexc: sigrt_handler SUSPEND\n");

	thread_t thread = mach_thread_self();
	state_to_kernel(ctxt, &tstate, &fstate);

	int ret = dserver_rpc_thread_suspended(&tstate, &fstate);
	if (ret < 0) {
		__simple_printf("dserver_rpc_thread_suspended failed internally: %d", ret);
		__simple_abort();
	}

	state_from_kernel(ctxt, &tstate, &fstate);
	} else if (signum == SIGNAL_S2C) {
		__simple_kprintf("sigexc: sigrt_handler S2C");

		int ret = dserver_rpc_s2c_perform();
		if (ret < 0) {
			__simple_printf("dserver_rpc_s2c_perform failed internally: %d", ret);
			__simple_abort();
		}
	} else {
		__simple_printf("Unknown/unrecognized real-time signal: %d", signum);
	}

	status = dserver_rpc_interrupt_exit();

	if (status != 0) {
		__simple_printf("*** dserver_rpc_interrupt_exit failed with code %d ***\n", status);
		__simple_abort();
	}
}


static void state_to_kernel(struct linux_ucontext* ctxt, void* tstate, void* fstate)
{
#if defined(__x86_64__)

	dump_gregs(&ctxt->uc_mcontext.gregs);
	mcontext_to_thread_state(&ctxt->uc_mcontext.gregs, (x86_thread_state64_t*) tstate);
	mcontext_to_float_state(ctxt->uc_mcontext.fpregs, (x86_float_state64_t*) fstate);

#elif defined(__i386__)
	mcontext_to_thread_state(&ctxt->uc_mcontext.gregs, (x86_thread_state32_t*) tstate);
	mcontext_to_float_state(ctxt->uc_mcontext.fpregs, (x86_float_state32_t*) fstate);
#endif

}

static void state_from_kernel(struct linux_ucontext* ctxt, const void* tstate, const void* fstate)
{
#if defined(__x86_64__)

	thread_state_to_mcontext((const x86_thread_state64_t*) tstate, &ctxt->uc_mcontext.gregs);
	float_state_to_mcontext((const x86_float_state64_t*) fstate, ctxt->uc_mcontext.fpregs);

	dump_gregs(&ctxt->uc_mcontext.gregs);

#elif defined(__i386__)
	thread_state_to_mcontext((const x86_thread_state32_t*) tstate, &ctxt->uc_mcontext.gregs);
	float_state_to_mcontext((const x86_float_state32_t*) fstate, ctxt->uc_mcontext.fpregs);
#endif
}

void sigexc_handler(int linux_signum, struct linux_siginfo* info, struct linux_ucontext* ctxt)
{
	int status = dserver_rpc_interrupt_enter();

	if (status != 0) {
		__simple_printf("*** dserver_rpc_interrupt_enter failed with code %d ***\n", status);
		__simple_abort();
	}

	kern_printf("sigexc_handler(%d, %p, %p)\n", linux_signum, info, ctxt);


	if (linux_signum == LINUX_SIGCONT)
		goto out;

	int bsd_signum = signum_linux_to_bsd(linux_signum);
	if (bsd_signum <= 0)
	{
		kern_printf("sigexc: Unmapped signal!\n");
		goto out;
	}

#ifdef __x86_64__
	kern_printf("sigexc: have RIP 0x%llx\n", ctxt->uc_mcontext.gregs.rip);
#endif

	thread_t thread = mach_thread_self();

#if defined(__x86_64__)
	x86_thread_state64_t tstate;
	x86_float_state64_t fstate;
#elif defined(__i386__)
	x86_thread_state32_t tstate;
	x86_float_state32_t fstate;
#endif

	state_to_kernel(ctxt, &tstate, &fstate);
	int ret = dserver_rpc_sigprocess(bsd_signum, linux_signum, info->si_pid, info->si_code, info->si_addr, &tstate, &fstate, &bsd_signum);
	if (ret < 0) {
		__simple_printf("sigprocess failed internally while processing Linux signal %d: %d", linux_signum, ret);
		__simple_abort();
	}
	state_from_kernel(ctxt, &tstate, &fstate);

	if (!bsd_signum)
	{
		kern_printf("sigexc: drop signal\n");
		goto out;
	}

	linux_signum = signum_bsd_to_linux(bsd_signum);

	if (sig_handlers[linux_signum] != BSD_SIG_IGN)
	{
		if (sig_handlers[linux_signum])
		{
			kern_printf("sigexc: will forward signal to app handler (%p)\n", sig_handlers[linux_signum]);

			// Update the signal mask to what the application actually wanted
			linux_sigset_t set = sig_masks[linux_signum];

			// Keep the current signal blocked, which is the default behavior
			if (!(sig_flags[linux_signum] & LINUX_SA_NODEFER))
				set |= (1ull << (linux_signum-1));

			LINUX_SYSCALL(__NR_rt_sigprocmask, 2 /* LINUX_SIG_SETMASK */,
				&set, NULL, sizeof(linux_sigset_t));

			handler_linux_to_bsd(linux_signum, info, ctxt);
		}
		else
		{
			if (bsd_signum == SIGTSTP || bsd_signum == SIGSTOP)
			{
				kern_printf("sigexc: emulating SIGTSTP/SIGSTOP\n");
				LINUX_SYSCALL(__NR_kill, 0, LINUX_SIGSTOP);
			}
			else
			{
				kern_printf("sigexc: emulating default signal effects\n");
				// Set handler to BSD_SIG_DFL
				struct linux_sigaction sa;
				sa.sa_sigaction = (linux_sig_handler*) NULL; // BSD_SIG_DFL
				sa.sa_mask = 0x7fffffff; // all other standard Unix signals should be blocked while the handler is run
				sa.sa_flags = LINUX_SA_RESTORER | LINUX_SA_SIGINFO | LINUX_SA_RESTART | LINUX_SA_ONSTACK;
				sa.sa_restorer = linux_sig_restorer;

				LINUX_SYSCALL(__NR_rt_sigaction, linux_signum,
						&sa, NULL,
						sizeof(sa.sa_mask));

				// Resend signal to self
				LINUX_SYSCALL(__NR_kill, 0, linux_signum);
			}
		}
	}

	kern_printf("sigexc: handler (%d) returning\n", linux_signum);

out:
	status = dserver_rpc_interrupt_exit();

	if (status != 0) {
		__simple_printf("*** dserver_rpc_interrupt_exit failed with code %d ***\n", status);
		__simple_abort();
	}
}

void sigexc_thread_setup(void)
{
	struct bsd_stack newstack = {
		.ss_size = default_sigaltstack_size,
		.ss_flags = 0
	};

#if SIGALTSTACK_GUARD
	newstack.ss_sp = (void*) sys_mmap(NULL, newstack.ss_size + 4096, PROT_READ | PROT_WRITE,
			MAP_ANON | MAP_PRIVATE, -1, 0);
	sys_mprotect(newstack.ss_sp, 4096, PROT_NONE);
	newstack.ss_sp = (char*)newstack.ss_sp + 4096;
#else
	newstack.ss_sp = (void*) sys_mmap(NULL, newstack.ss_size, PROT_READ | PROT_WRITE,
			MAP_ANON | MAP_PRIVATE, -1, 0);
#endif
	sys_sigaltstack(&newstack, NULL);
}

void sigexc_thread_exit(void)
{
	struct bsd_stack oldstack;
	sys_sigaltstack(NULL, &oldstack);

#if SIGALTSTACK_GUARD
	sys_munmap((char*)oldstack.ss_sp - 4096, oldstack.ss_size + 4096);
#else
	sys_munmap(oldstack.ss_sp, oldstack.ss_size);
#endif
}

