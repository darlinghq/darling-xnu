// sigaction(2) flags that change how a handler is run:
// - SA_NODEFER: the signal is not blocked while its handler runs, so the handler is entered again when the
//   signal arrives during it; without the flag the second delivery waits until the handler returns.
// - SA_RESETHAND: the disposition goes back to SIG_DFL when the handler is entered (except for SIGILL and
//   SIGTRAP), so the next delivery takes the default action.
// - SA_ONSTACK: the handler runs on the alternate stack the thread installed with sigaltstack(), if it is
//   not on it already. Without the flag, or without an alternate stack, the handler runs on the
//   interrupted stack, below the interrupted stack pointer and its 128-byte red zone.
#include <pthread.h>
#include <setjmp.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

static int failures;

static void check(int ok, const char* what, const char* detail)
{
	if (!ok) {
		printf("FAIL: %s: %s\n", what, detail);
		failures++;
	} else {
		printf("ok: %s\n", what);
	}
}

static void install(int sig, void (*handler)(int, siginfo_t*, void*), int flags)
{
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sigemptyset(&sa.sa_mask);
	sa.sa_sigaction = handler;
	sa.sa_flags = SA_SIGINFO | flags;
	if (sigaction(sig, &sa, NULL) != 0) {
		perror("sigaction");
		exit(2);
	}
}

static void reset_default(int sig)
{
	signal(sig, SIG_DFL);
}

// --- nesting: the handler raises its own signal once

static volatile sig_atomic_t runs, depth, max_depth;

static void raising_handler(int sig, siginfo_t* info, void* uc)
{
	(void) info;
	(void) uc;
	runs++;
	depth++;
	if (depth > max_depth)
		max_depth = depth;
	if (runs == 1)
		raise(sig);
	depth--;
}

static void test_nesting(int nodefer)
{
	char what[96], detail[128];
	runs = depth = max_depth = 0;
	install(SIGUSR1, raising_handler, nodefer ? SA_NODEFER : 0);
	raise(SIGUSR1);
	reset_default(SIGUSR1);
	snprintf(what, sizeof(what), "%s: the handler raising its own signal %s", nodefer ? "SA_NODEFER" : "no SA_NODEFER",
		nodefer ? "is entered again" : "waits until it returns");
	snprintf(detail, sizeof(detail), "%d runs, maximum depth %d; expected 2 runs, depth %d", (int) runs,
		(int) max_depth, nodefer ? 2 : 1);
	check(runs == 2 && max_depth == (nodefer ? 2 : 1), what, detail);
}

// --- SA_RESETHAND

static void counting_handler(int sig, siginfo_t* info, void* uc)
{
	(void) sig;
	(void) info;
	(void) uc;
	runs++;
}

static void test_resethand_disposition(int sig, int expect_reset)
{
	char what[96], detail[128];
	struct sigaction old;
	runs = 0;
	install(sig, counting_handler, SA_RESETHAND);
	raise(sig);
	sigaction(sig, NULL, &old);
	int is_default = old.sa_handler == SIG_DFL;
	snprintf(what, sizeof(what), "SA_RESETHAND on signal %d: the disposition %s after the first delivery", sig,
		expect_reset ? "is SIG_DFL" : "stays the handler");
	snprintf(detail, sizeof(detail), "handler ran %d times, disposition %s SIG_DFL", (int) runs,
		is_default ? "is" : "is not");
	check(runs == 1 && is_default == expect_reset, what, detail);
	reset_default(sig);
}

// The second delivery takes the default action: SIGUSR1 ends the process, SIGWINCH is ignored.
static void test_resethand_default_action(void)
{
	pid_t child = fork();
	if (child < 0) {
		perror("fork");
		exit(2);
	}
	if (child == 0) {
		runs = 0;
		install(SIGUSR1, counting_handler, SA_RESETHAND);
		raise(SIGUSR1);
		if (runs != 1)
			_exit(3);
		raise(SIGUSR1);
		_exit(4); // still alive: the handler was not reset
	}
	int status;
	waitpid(child, &status, 0);
	char detail[96];
	snprintf(detail, sizeof(detail), "child status 0x%x; expected killed by signal %d", status, SIGUSR1);
	check(WIFSIGNALED(status) && WTERMSIG(status) == SIGUSR1, "SA_RESETHAND: the second SIGUSR1 ends the process",
		detail);

	runs = 0;
	install(SIGWINCH, counting_handler, SA_RESETHAND);
	raise(SIGWINCH);
	raise(SIGWINCH);
	snprintf(detail, sizeof(detail), "handler ran %d times; expected once", (int) runs);
	check(runs == 1, "SA_RESETHAND: the second SIGWINCH is ignored", detail);
	reset_default(SIGWINCH);
}

// --- the stack a handler runs on

#define MAX_DEPTH 2
// how far below the interrupted stack pointer a handler's frame may be (the kernel's frame comes first)
#define FRAME_ALLOWANCE (64 * 1024)
#define RED_ZONE 128

static char altstack[64 * 1024] __attribute__((aligned(16)));
static volatile uintptr_t handler_sp[MAX_DEPTH], interrupted_sp[MAX_DEPTH];

static uintptr_t context_sp(void* uc)
{
	mcontext_t mc = ((ucontext_t*) uc)->uc_mcontext;
#if defined(__x86_64__)
	return mc->__ss.__rsp;
#elif defined(__i386__)
	return mc->__ss.__esp;
#elif defined(__arm64__)
	return mc->__ss.__sp;
#else
#error unsupported architecture
#endif
}

static void recording_handler(int sig, siginfo_t* info, void* uc)
{
	(void) info;
	volatile char local;
	int level = depth++;
	if (level < MAX_DEPTH) {
		handler_sp[level] = (uintptr_t) &local;
		interrupted_sp[level] = context_sp(uc);
	}
	runs++;
	if (depth > max_depth)
		max_depth = depth;
	// with SA_NODEFER, a second level
	if (runs == 1 && sig == SIGUSR2)
		raise(sig);
	depth--;
}

static int on_altstack(uintptr_t sp)
{
	return sp >= (uintptr_t) altstack && sp < (uintptr_t) altstack + sizeof(altstack);
}

static int on_interrupted_stack(int level)
{
	uintptr_t top = interrupted_sp[level] - RED_ZONE;
	return handler_sp[level] < top && handler_sp[level] >= top - FRAME_ALLOWANCE && !on_altstack(handler_sp[level]);
}

enum where { INTERRUPTED, ALTSTACK };

// Raises sig; for SIGUSR2, whose handler raises it again, with SA_NODEFER: two levels.
static void test_stack_of(const char* what, int sig, int flags, enum where expected)
{
	char detail[256];
	int levels = (sig == SIGUSR2 && (flags & SA_NODEFER)) ? 2 : 1;
	runs = depth = max_depth = 0;
	memset((void*) handler_sp, 0, sizeof(handler_sp));
	memset((void*) interrupted_sp, 0, sizeof(interrupted_sp));
	install(sig, recording_handler, flags);
	raise(sig);
	reset_default(sig);

	int ok = runs == levels && max_depth == levels;
	for (int i = 0; i < levels; i++)
		ok = ok && (expected == ALTSTACK ? on_altstack(handler_sp[i]) : on_interrupted_stack(i));
	if (levels == 2)
		ok = ok && handler_sp[1] < handler_sp[0];
	snprintf(detail, sizeof(detail), "%d runs, depth %d; handler at %#lx, %#lx; interrupted at %#lx, %#lx; "
		"alternate stack %p-%p", (int) runs, (int) max_depth, (unsigned long) handler_sp[0],
		(unsigned long) handler_sp[1], (unsigned long) interrupted_sp[0], (unsigned long) interrupted_sp[1],
		(void*) altstack, (void*) (altstack + sizeof(altstack)));
	check(ok, what, detail);
}

// Raises SIGUSR1 (one level) or, with SA_NODEFER, SIGUSR2 (two levels).
static void test_stack(const char* what, int flags, enum where expected)
{
	test_stack_of(what, (flags & SA_NODEFER) ? SIGUSR2 : SIGUSR1, flags, expected);
}

// A thread that never called sigaltstack() has no alternate stack.
static void* thread_without_altstack(void* arg)
{
	(void) arg;
	test_stack("new thread without an alternate stack: an SA_ONSTACK handler runs on the interrupted stack",
		SA_ONSTACK, INTERRUPTED);
	test_stack("new thread without an alternate stack: nested SA_ONSTACK|SA_NODEFER handlers run on the "
		"interrupted stack", SA_ONSTACK | SA_NODEFER, INTERRUPTED);
	return NULL;
}

static void overflow_handler(int sig, siginfo_t* info, void* uc)
{
	(void) sig;
	(void) info;
	(void) uc;
	volatile char local;
	_exit(on_altstack((uintptr_t) &local) ? 42 : 43);
}

static int recurse(int n)
{
	volatile char frame[4096];
	frame[0] = (char) n;
	if (n == INT32_MAX) // never: the stack runs out first
		return 0;
	return recurse(n + 1) + frame[0];
}

// A stack overflow is caught by a SIGSEGV handler on the alternate stack.
static void test_stack_overflow(void)
{
	pid_t child = fork();
	if (child < 0) {
		perror("fork");
		exit(2);
	}
	if (child == 0) {
		stack_t ss = { .ss_sp = altstack, .ss_size = sizeof(altstack), .ss_flags = 0 };
		sigaltstack(&ss, NULL);
		install(SIGSEGV, overflow_handler, SA_ONSTACK);
		install(SIGBUS, overflow_handler, SA_ONSTACK);
		_exit(recurse(0) == 0 ? 3 : 4);
	}
	int status;
	waitpid(child, &status, 0);
	char detail[96];
	snprintf(detail, sizeof(detail), "child status 0x%x; expected exit 42", status);
	check(WIFEXITED(status) && WEXITSTATUS(status) == 42,
		"stack overflow: the SA_ONSTACK SIGSEGV handler runs on the alternate stack", detail);
}

// --- many signals on a thread without an alternate stack, whose SA_ONSTACK handlers run on the
// interrupted stack: a 1 ms profiling timer, signals from another thread, and an ignored one

static volatile sig_atomic_t prof_runs, usr1_runs;
static volatile int stress_done;

static void prof_handler(int sig, siginfo_t* info, void* uc)
{
	(void) sig;
	(void) info;
	(void) uc;
	prof_runs++;
}

static void usr1_handler(int sig, siginfo_t* info, void* uc)
{
	(void) sig;
	(void) info;
	(void) uc;
	usr1_runs++;
}

static double now_seconds(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec + ts.tv_nsec / 1e9;
}

static void* stress_worker(void* arg)
{
	(void) arg;
	// values a handler's frame would overwrite if it were put on top of this thread's live frames
	volatile uint32_t canary[256];
	for (unsigned i = 0; i < 256; i++)
		canary[i] = 0x5a5a0000u + i;
	int intact = 1;
	double end = now_seconds() + 2.0;
	while (now_seconds() < end) {
		for (unsigned i = 0; i < 256; i++)
			intact &= canary[i] == 0x5a5a0000u + i;
	}
	stress_done = 1;
	return (void*) (uintptr_t) intact;
}

static void test_signal_stress(void)
{
	prof_runs = usr1_runs = 0;
	stress_done = 0;
	install(SIGPROF, prof_handler, SA_ONSTACK | SA_RESTART);
	install(SIGUSR1, usr1_handler, SA_ONSTACK);
	signal(SIGWINCH, SIG_IGN);

	pthread_t worker;
	pthread_create(&worker, NULL, stress_worker, NULL);
	struct itimerval it = { { 0, 1000 }, { 0, 1000 } };
	setitimer(ITIMER_PROF, &it, NULL);
	while (!stress_done) {
		pthread_kill(worker, SIGUSR1);
		pthread_kill(worker, SIGWINCH);
		struct timespec pause = { 0, 200000 };
		nanosleep(&pause, NULL);
	}
	void* intact;
	pthread_join(worker, &intact);
	struct itimerval off = { { 0, 0 }, { 0, 0 } };
	setitimer(ITIMER_PROF, &off, NULL);
	reset_default(SIGPROF);
	reset_default(SIGUSR1);
	reset_default(SIGWINCH);

	char detail[128];
	snprintf(detail, sizeof(detail), "%d SIGPROF and %d SIGUSR1 handled, the thread's frames %s",
		(int) prof_runs, (int) usr1_runs, intact ? "intact" : "overwritten");
	check(intact && prof_runs > 100 && usr1_runs > 100,
		"thousands of SA_ONSTACK handlers on threads without an alternate stack", detail);
}

// --- a fault handler without SA_ONSTACK runs on the interrupted stack too

static char* fault_page;

static void fault_handler(int sig, siginfo_t* info, void* uc)
{
	(void) sig;
	(void) info;
	volatile char local;
	handler_sp[0] = (uintptr_t) &local;
	interrupted_sp[0] = context_sp(uc);
	runs++;
	mprotect(fault_page, (size_t) getpagesize(), PROT_READ | PROT_WRITE);
}

static void test_fault_stack(const char* what)
{
	char detail[160];
	fault_page = mmap(NULL, (size_t) getpagesize(), PROT_NONE, MAP_ANON | MAP_PRIVATE, -1, 0);
	runs = 0;
	install(SIGSEGV, fault_handler, 0);
	install(SIGBUS, fault_handler, 0);
	*(volatile char*) fault_page = 1;
	reset_default(SIGSEGV);
	reset_default(SIGBUS);
	munmap(fault_page, (size_t) getpagesize());
	snprintf(detail, sizeof(detail), "%d runs; handler at %#lx, interrupted at %#lx", (int) runs,
		(unsigned long) handler_sp[0], (unsigned long) interrupted_sp[0]);
	check(runs == 1 && on_interrupted_stack(0), what, detail);
}

// --- the alternate stack after a handler: what the handler left, also when it left by longjmp()

static char other_altstack[64 * 1024] __attribute__((aligned(16)));
static sigjmp_buf escape;

static void escaping_handler(int sig, siginfo_t* info, void* uc)
{
	(void) sig;
	(void) info;
	(void) uc;
	siglongjmp(escape, 1);
}

static void installing_handler(int sig, siginfo_t* info, void* uc)
{
	(void) sig;
	(void) info;
	(void) uc;
	stack_t ss = { .ss_sp = other_altstack, .ss_size = sizeof(other_altstack), .ss_flags = 0 };
	sigaltstack(&ss, NULL);
}

static void disabling_handler(int sig, siginfo_t* info, void* uc)
{
	(void) sig;
	(void) info;
	(void) uc;
	// Libc refuses a size below MINSIGSTKSZ even with SS_DISABLE, as on macOS
	stack_t ss = { .ss_sp = other_altstack, .ss_size = sizeof(other_altstack), .ss_flags = SS_DISABLE };
	sigaltstack(&ss, NULL);
	if (fault_page != NULL)
		mprotect(fault_page, (size_t) getpagesize(), PROT_READ | PROT_WRITE);
}

static int current_altstack_is(void* sp)
{
	stack_t cur;
	if (sigaltstack(NULL, &cur) != 0)
		return 0;
	return sp == NULL ? (cur.ss_flags & SS_DISABLE) != 0 : !(cur.ss_flags & SS_DISABLE) && cur.ss_sp == sp;
}

static void* altstack_after_handlers(void* arg)
{
	(void) arg;
	// a thread without an alternate stack: an SA_ONSTACK handler that installs one keeps it
	install(SIGUSR1, installing_handler, SA_ONSTACK);
	raise(SIGUSR1);
	check(current_altstack_is(other_altstack), "a handler that installs an alternate stack keeps it",
		"sigaltstack() does not report the handler's stack after it returned");

	// a handler without SA_ONSTACK, run on this thread's stack, that installs another one keeps it
	stack_t ss = { .ss_sp = altstack, .ss_size = sizeof(altstack), .ss_flags = 0 };
	sigaltstack(&ss, NULL);
	install(SIGUSR1, installing_handler, 0);
	raise(SIGUSR1);
	check(current_altstack_is(other_altstack), "a handler without SA_ONSTACK that installs an alternate stack "
		"keeps it", "sigaltstack() does not report the handler's stack after it returned");

	// a SIGSEGV handler without SA_ONSTACK that disables the alternate stack leaves it disabled
	fault_page = mmap(NULL, (size_t) getpagesize(), PROT_NONE, MAP_ANON | MAP_PRIVATE, -1, 0);
	install(SIGSEGV, disabling_handler, 0);
	install(SIGBUS, disabling_handler, 0);
	*(volatile char*) fault_page = 1;
	reset_default(SIGSEGV);
	reset_default(SIGBUS);
	munmap(fault_page, (size_t) getpagesize());
	fault_page = NULL;
	check(current_altstack_is(NULL), "a SIGSEGV handler that disables the alternate stack leaves it disabled",
		"sigaltstack() reports a stack after the handler disabled it");
	reset_default(SIGUSR1);
	return NULL;
}

// A handler left by longjmp() hands the alternate stack back to the next signal that interrupts the
// thread above where the handler started: a signal that arrives while the thread spins in a shallow
// frame, after one that started in a deep frame.
static void __attribute__((noinline)) deep_fault(void)
{
	volatile char pad[16384];
	pad[0] = 0;
	*(volatile char*) fault_page = 1;
	(void) pad[0];
}

static volatile int spinner_ready;

static void* altstack_after_longjmp(void* arg)
{
	(void) arg;
	// a thread without an alternate stack whose SA_ONSTACK handler leaves by longjmp(), then installs one
	fault_page = mmap(NULL, (size_t) getpagesize(), PROT_NONE, MAP_ANON | MAP_PRIVATE, -1, 0);
	// macOS reports an access to a PROT_NONE page as SIGBUS, Linux as SIGSEGV
	install(SIGSEGV, escaping_handler, SA_ONSTACK);
	install(SIGBUS, escaping_handler, SA_ONSTACK);
	if (!sigsetjmp(escape, 1))
		deep_fault();
	reset_default(SIGSEGV);
	reset_default(SIGBUS);
	munmap(fault_page, (size_t) getpagesize());
	fault_page = NULL;
	stack_t ss = { .ss_sp = other_altstack, .ss_size = sizeof(other_altstack), .ss_flags = 0 };
	sigaltstack(&ss, NULL);

	// a signal from another thread, taken while this one spins, then two on the alternate stack
	runs = 0;
	install(SIGUSR2, counting_handler, 0);
	spinner_ready = 1;
	while (runs == 0)
		;
	reset_default(SIGUSR2);
	runs = depth = max_depth = 0;
	install(SIGUSR1, recording_handler, SA_ONSTACK);
	raise(SIGUSR1);
	raise(SIGUSR1);
	reset_default(SIGUSR1);
	int on_own = handler_sp[0] >= (uintptr_t) other_altstack
		&& handler_sp[0] < (uintptr_t) other_altstack + sizeof(other_altstack);
	check(on_own && current_altstack_is(other_altstack),
		"after a handler left by longjmp(), the alternate stack installed next stays", "it was replaced");
	return NULL;
}

// The application's stack parked for a SIGSEGV handler that left by longjmp(), then disabled and
// made inaccessible: no signal may bring it back (the child would die writing a frame there).
static void test_disabled_after_longjmp(void)
{
	pid_t child = fork();
	if (child < 0) {
		perror("fork");
		exit(2);
	}
	if (child == 0) {
		char* stack = mmap(NULL, 64 * 1024, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
		stack_t ss = { .ss_sp = stack, .ss_size = 64 * 1024, .ss_flags = 0 };
		sigaltstack(&ss, NULL);
		fault_page = mmap(NULL, (size_t) getpagesize(), PROT_NONE, MAP_ANON | MAP_PRIVATE, -1, 0);
		install(SIGSEGV, escaping_handler, 0);
		install(SIGBUS, escaping_handler, 0);
		if (!sigsetjmp(escape, 1))
			deep_fault();
		ss.ss_flags = SS_DISABLE;
		sigaltstack(&ss, NULL);
		mprotect(stack, 64 * 1024, PROT_NONE);
		runs = 0;
		install(SIGALRM, counting_handler, SA_ONSTACK);
		struct itimerval it = { { 0, 0 }, { 0, 50000 } };
		setitimer(ITIMER_REAL, &it, NULL);
		while (runs == 0)
			;
		install(SIGUSR1, recording_handler, SA_ONSTACK);
		raise(SIGUSR1);
		raise(SIGUSR1);
		_exit(current_altstack_is(NULL) ? 0 : 5);
	}
	int status;
	waitpid(child, &status, 0);
	char detail[96];
	snprintf(detail, sizeof(detail), "child status 0x%x; expected exit 0", status);
	check(WIFEXITED(status) && WEXITSTATUS(status) == 0,
		"an alternate stack disabled after a handler left by longjmp() stays disabled", detail);
}

static int sigchld_runs;
static uintptr_t sigchld_sp;
static volatile int sigchld_spinning, sigchld_gave_up;

// After a handler without SA_ONSTACK that ran off the alternate stack left by longjmp(), SIGCHLD's
// handler gives the stack back as well: the next SA_ONSTACK one runs on it.
static void* sigchld_after_longjmp(void* arg)
{
	(void) arg;
	stack_t ss = { .ss_sp = altstack, .ss_size = sizeof(altstack), .ss_flags = 0 };
	sigaltstack(&ss, NULL);
	fault_page = mmap(NULL, (size_t) getpagesize(), PROT_NONE, MAP_ANON | MAP_PRIVATE, -1, 0);
	install(SIGSEGV, escaping_handler, 0);
	install(SIGBUS, escaping_handler, 0);
	if (!sigsetjmp(escape, 1))
		deep_fault();
	reset_default(SIGSEGV);
	reset_default(SIGBUS);
	munmap(fault_page, (size_t) getpagesize());
	fault_page = NULL;
	runs = depth = max_depth = 0;
	install(SIGCHLD, recording_handler, SA_ONSTACK);
	// The signals come from another thread while this one spins in a shallow frame (raise() would take
	// them deeper down the stack than the handler left by longjmp() started).
	sigchld_spinning = 1;
	while (runs < 2 && !sigchld_gave_up)
		;
	reset_default(SIGCHLD);
	int ok = runs == 2 && on_altstack(handler_sp[0]);
	sigchld_runs = runs;
	sigchld_sp = handler_sp[0];
	ss.ss_flags = SS_DISABLE;
	sigaltstack(&ss, NULL);
	return (void*) (uintptr_t) ok;
}

static void test_altstack_after_handlers(void)
{
	pthread_t thread;
	pthread_create(&thread, NULL, altstack_after_handlers, NULL);
	pthread_join(thread, NULL);
	spinner_ready = 0;
	pthread_create(&thread, NULL, altstack_after_longjmp, NULL);
	while (!spinner_ready)
		;
	pthread_kill(thread, SIGUSR2);
	pthread_join(thread, NULL);
	test_disabled_after_longjmp();

	void* ok;
	pthread_create(&thread, NULL, sigchld_after_longjmp, NULL);
	while (!sigchld_spinning)
		;
	// one at a time: a second SIGCHLD sent while the first is pending would be merged with it
	for (int i = 1; i <= 2; i++) {
		pthread_kill(thread, SIGCHLD);
		for (int wait = 0; runs < i && wait < 2000; wait++) {
			struct timespec ms = { 0, 1000000 };
			nanosleep(&ms, NULL);
		}
	}
	sigchld_gave_up = 1;
	pthread_join(thread, &ok);
	char detail[160];
	snprintf(detail, sizeof(detail), "%d runs, the second at %#lx; alternate stack %p-%p", sigchld_runs,
		(unsigned long) sigchld_sp, (void*) altstack, (void*) (altstack + sizeof(altstack)));
	check(ok != NULL, "after a handler left by longjmp(), an SA_ONSTACK SIGCHLD handler gets the alternate stack back",
		detail);
}

static void test_stacks(void)
{
	// The main thread has not called sigaltstack() yet.
	test_stack("no alternate stack: an SA_ONSTACK handler runs on the interrupted stack", SA_ONSTACK, INTERRUPTED);
	test_stack("no alternate stack: a handler without SA_ONSTACK runs on the interrupted stack", 0, INTERRUPTED);
	test_stack("no alternate stack: nested SA_ONSTACK|SA_NODEFER handlers run on the interrupted stack",
		SA_ONSTACK | SA_NODEFER, INTERRUPTED);

	pthread_t thread;
	pthread_create(&thread, NULL, thread_without_altstack, NULL);
	pthread_join(thread, NULL);

	test_signal_stress();
	test_fault_stack("no alternate stack: a SIGSEGV handler without SA_ONSTACK runs on the interrupted stack");
	test_stack_of("no alternate stack: an SA_ONSTACK SIGCHLD handler runs on the interrupted stack", SIGCHLD,
		SA_ONSTACK, INTERRUPTED);
	test_stack_of("no alternate stack: an SA_ONSTACK SIGTTIN handler runs on the interrupted stack", SIGTTIN,
		SA_ONSTACK, INTERRUPTED);

	stack_t ss = { .ss_sp = altstack, .ss_size = sizeof(altstack), .ss_flags = 0 };
	if (sigaltstack(&ss, NULL) != 0) {
		perror("sigaltstack");
		exit(2);
	}
	test_stack("an SA_ONSTACK handler runs on the alternate stack", SA_ONSTACK, ALTSTACK);
	test_stack("a handler without SA_ONSTACK runs on the interrupted stack", 0, INTERRUPTED);
	test_stack("nested SA_ONSTACK|SA_NODEFER handlers run on the alternate stack", SA_ONSTACK | SA_NODEFER, ALTSTACK);
	test_stack("nested SA_NODEFER handlers without SA_ONSTACK run on the interrupted stack", SA_NODEFER,
		INTERRUPTED);
	test_fault_stack("a SIGSEGV handler without SA_ONSTACK runs on the interrupted stack");
	test_stack_of("an SA_ONSTACK SIGCHLD handler runs on the alternate stack", SIGCHLD, SA_ONSTACK, ALTSTACK);
	test_stack_of("a SIGCHLD handler without SA_ONSTACK runs on the interrupted stack", SIGCHLD, 0, INTERRUPTED);

	ss.ss_flags = SS_DISABLE;
	sigaltstack(&ss, NULL);
	test_stack("alternate stack disabled: an SA_ONSTACK handler runs on the interrupted stack", SA_ONSTACK,
		INTERRUPTED);

	test_stack_overflow();
	test_altstack_after_handlers();
}

int main(void)
{
	setvbuf(stdout, NULL, _IOLBF, 0);

	test_stacks();

	test_nesting(1);
	test_nesting(0);

	test_resethand_disposition(SIGUSR2, 1);
	test_resethand_disposition(SIGILL, 0);
	test_resethand_default_action();


	if (failures) {
		printf("%d failure(s)\n", failures);
		return 1;
	}
	printf("PASS\n");
	return 0;
}
