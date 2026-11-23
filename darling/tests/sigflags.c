// sigaction(2) flags that change how a handler is run:
// - SA_NODEFER: the signal is not blocked while its handler runs, so the handler is entered again when the
//   signal arrives during it; without the flag the second delivery waits until the handler returns.
// - SA_RESETHAND: the disposition goes back to SIG_DFL when the handler is entered (except for SIGILL and
//   SIGTRAP), so the next delivery takes the default action.
// - SA_ONSTACK: the handler runs on the alternate stack the thread installed with sigaltstack(), if it is
//   not on it already. Without the flag, or without an alternate stack, the handler runs on the
//   interrupted stack, below the interrupted stack pointer and its 128-byte red zone.
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
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

// Raises SIGUSR1 (one level) or, with SA_NODEFER, SIGUSR2 (whose handler raises it again: two levels).
static void test_stack(const char* what, int flags, enum where expected)
{
	char detail[256];
	int sig = (flags & SA_NODEFER) ? SIGUSR2 : SIGUSR1;
	int levels = (flags & SA_NODEFER) ? 2 : 1;
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

static void test_stacks(void)
{
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

	test_stack_overflow();
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
