// What sigaction(2) and sigaltstack(2) report back, following XNU (bsd/kern/kern_sig.c):
// - sigaction() reports the flags XNU keeps for a signal: SA_ONSTACK, SA_RESTART, SA_SIGINFO, SA_NODEFER,
//   and for SIGCHLD only SA_NOCLDSTOP and SA_NOCLDWAIT (also reported after SIGCHLD was set to SIG_IGN,
//   which makes children be reaped as with SA_NOCLDWAIT). Never SA_RESETHAND. A signal never set reports
//   SA_RESTART: XNU records which signals interrupt system calls, and none does until an action without
//   SA_RESTART is set. The mask is reported without SIGKILL and SIGSTOP, which cannot be blocked.
// - after SA_RESETHAND took effect, the action is SIG_DFL without SA_SIGINFO and SA_NODEFER, also for the
//   signals whose default action is to ignore them (SIGCHLD, SIGURG, SIGWINCH), which are not SIG_IGN.
//   Those, and SIGIO and SIGINFO, are ignored by default: raising them does not end the process.
// - sigaltstack() reports the alternate stack the thread installed, SS_DISABLE with no address and size
//   when it installed none, also inside a handler; and the installed one, also while a handler without
//   SA_ONSTACK runs on the interrupted stack.
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
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

static volatile sig_atomic_t runs;

static void counting_handler(int sig, siginfo_t* info, void* uc)
{
	(void) sig;
	(void) info;
	(void) uc;
	runs++;
}

static void install(int sig, int flags, sigset_t mask)
{
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_sigaction = counting_handler;
	sa.sa_mask = mask;
	sa.sa_flags = flags;
	if (sigaction(sig, &sa, NULL) != 0) {
		perror("sigaction");
		exit(2);
	}
}

static void check_action(int sig, void* handler, int flags, const char* what)
{
	struct sigaction old;
	char detail[160];
	memset(&old, 0xa5, sizeof(old));
	sigaction(sig, NULL, &old);
	snprintf(detail, sizeof(detail), "handler %p, flags %#x; expected %p, %#x", (void*) old.sa_handler,
		old.sa_flags, handler, flags);
	check((void*) old.sa_handler == handler && old.sa_flags == flags, what, detail);
}

// --- sigaction() flags

static void test_flags(void)
{
	sigset_t none;
	sigemptyset(&none);

	// nothing in this process sets SIGXCPU before
	check_action(SIGXCPU, (void*) SIG_DFL, SA_RESTART, "a signal never set reports SIG_DFL with SA_RESTART");

	install(SIGUSR1, SA_SIGINFO | SA_RESETHAND | SA_NODEFER | SA_ONSTACK | SA_RESTART, none);
	check_action(SIGUSR1, (void*) counting_handler, SA_SIGINFO | SA_NODEFER | SA_ONSTACK | SA_RESTART,
		"SA_RESETHAND is not reported");

	install(SIGUSR1, SA_SIGINFO | SA_NOCLDSTOP | SA_NOCLDWAIT, none);
	check_action(SIGUSR1, (void*) counting_handler, SA_SIGINFO,
		"SA_NOCLDSTOP and SA_NOCLDWAIT are not reported for a signal other than SIGCHLD");
	signal(SIGUSR1, SIG_DFL);

	install(SIGCHLD, SA_SIGINFO | SA_NOCLDSTOP | SA_RESETHAND, none);
	check_action(SIGCHLD, (void*) counting_handler, SA_SIGINFO | SA_NOCLDSTOP,
		"SIGCHLD reports SA_NOCLDSTOP, not SA_RESETHAND");

	struct sigaction ign;
	memset(&ign, 0, sizeof(ign));
	ign.sa_handler = SIG_IGN;
	sigaction(SIGCHLD, &ign, NULL);
	check_action(SIGCHLD, (void*) SIG_IGN, SA_NOCLDWAIT, "SIGCHLD set to SIG_IGN reports SA_NOCLDWAIT");
	signal(SIGCHLD, SIG_DFL);

	sigset_t mask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGKILL);
	sigaddset(&mask, SIGSTOP);
	sigaddset(&mask, SIGUSR2);
	install(SIGUSR1, SA_SIGINFO, mask);
	struct sigaction old;
	sigaction(SIGUSR1, NULL, &old);
	char detail[96];
	snprintf(detail, sizeof(detail), "mask %#x; expected %#x", (unsigned) old.sa_mask, (unsigned) sigmask(SIGUSR2));
	check(old.sa_mask == sigmask(SIGUSR2), "the mask is reported without SIGKILL and SIGSTOP", detail);
	signal(SIGUSR1, SIG_DFL);
}

// --- the action after SA_RESETHAND took effect

static void test_resethand(int sig)
{
	char what[128];
	sigset_t none;
	sigemptyset(&none);
	runs = 0;
	install(sig, SA_SIGINFO | SA_RESETHAND | SA_NODEFER | SA_RESTART, none);
	if (sig == SIGCHLD) {
		pid_t child = fork();
		if (child == 0)
			_exit(0);
		while (runs == 0)
			usleep(1000);
		waitpid(child, NULL, 0);
	} else {
		raise(sig);
	}
	snprintf(what, sizeof(what), "signal %d after SA_RESETHAND: SIG_DFL with SA_RESTART only", sig);
	check_action(sig, (void*) SIG_DFL, SA_RESTART, what);
	signal(sig, SIG_DFL);
}

static void test_ignored_by_default(void)
{
	char what[128];
	int sigs[] = { SIGCHLD, SIGURG, SIGWINCH };
	for (unsigned i = 0; i < sizeof(sigs) / sizeof(sigs[0]); i++) {
		snprintf(what, sizeof(what), "signal %d, ignored by default, reports SIG_DFL before it is set", sigs[i]);
		check_action(sigs[i], (void*) SIG_DFL, SA_RESTART, what);
	}

	pid_t child = fork();
	if (child < 0) {
		perror("fork");
		exit(2);
	}
	if (child == 0) {
		int raised[] = { SIGCHLD, SIGURG, SIGWINCH, SIGIO, SIGINFO };
		for (unsigned i = 0; i < sizeof(raised) / sizeof(raised[0]); i++) {
			signal(raised[i], SIG_DFL);
			raise(raised[i]);
		}
		_exit(0);
	}
	int status;
	waitpid(child, &status, 0);
	char detail[96];
	snprintf(detail, sizeof(detail), "child status 0x%x; expected exit 0", status);
	check(WIFEXITED(status) && WEXITSTATUS(status) == 0,
		"SIGCHLD, SIGURG, SIGWINCH, SIGIO and SIGINFO with SIG_DFL do not end the process", detail);
}

// --- sigaltstack()

static char altstack[64 * 1024] __attribute__((aligned(16)));
static stack_t in_handler;

static int is_none(const stack_t* ss)
{
	return ss->ss_flags == SS_DISABLE && ss->ss_sp == NULL && ss->ss_size == 0;
}

static int is_altstack(const stack_t* ss, int flags)
{
	return ss->ss_flags == flags && ss->ss_sp == altstack && ss->ss_size == sizeof(altstack);
}

static void describe(char* detail, size_t size, const stack_t* ss)
{
	snprintf(detail, size, "ss_sp %p, ss_size %#lx, ss_flags %#x (alternate stack %p, %#lx)", ss->ss_sp,
		(unsigned long) ss->ss_size, ss->ss_flags, (void*) altstack, (unsigned long) sizeof(altstack));
}

static void querying_handler(int sig, siginfo_t* info, void* uc)
{
	(void) sig;
	(void) info;
	(void) uc;
	sigaltstack(NULL, &in_handler);
	runs++;
}

static char* fault_page;

static void querying_fault_handler(int sig, siginfo_t* info, void* uc)
{
	querying_handler(sig, info, uc);
	mprotect(fault_page, (size_t) getpagesize(), PROT_READ | PROT_WRITE);
}

static void install_querying(int sig, void (*handler)(int, siginfo_t*, void*), int flags)
{
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_sigaction = handler;
	sa.sa_flags = SA_SIGINFO | flags;
	sigaction(sig, &sa, NULL);
}

static void* altstack_reports(void* arg)
{
	char detail[192];
	stack_t ss;
	const char* thread = arg;
	char what[160];

	memset(&ss, 0xa5, sizeof(ss));
	sigaltstack(NULL, &ss);
	describe(detail, sizeof(detail), &ss);
	snprintf(what, sizeof(what), "%s without an alternate stack: SS_DISABLE, no address or size", thread);
	check(is_none(&ss), what, detail);

	memset(&in_handler, 0xa5, sizeof(in_handler));
	install_querying(SIGUSR1, querying_handler, SA_ONSTACK);
	raise(SIGUSR1);
	describe(detail, sizeof(detail), &in_handler);
	snprintf(what, sizeof(what), "%s without an alternate stack, in an SA_ONSTACK handler: SS_DISABLE", thread);
	check(is_none(&in_handler), what, detail);

	stack_t mine = { .ss_sp = altstack, .ss_size = sizeof(altstack), .ss_flags = 0 };
	memset(&ss, 0xa5, sizeof(ss));
	sigaltstack(&mine, &ss);
	describe(detail, sizeof(detail), &ss);
	snprintf(what, sizeof(what), "%s: the stack the first sigaltstack() replaces is reported as none", thread);
	check(is_none(&ss), what, detail);

	sigaltstack(NULL, &ss);
	describe(detail, sizeof(detail), &ss);
	snprintf(what, sizeof(what), "%s: the installed alternate stack is reported", thread);
	check(is_altstack(&ss, 0), what, detail);

	raise(SIGUSR1);
	describe(detail, sizeof(detail), &in_handler);
	snprintf(what, sizeof(what), "%s: in an SA_ONSTACK handler, the alternate stack with SS_ONSTACK", thread);
	check(is_altstack(&in_handler, SS_ONSTACK), what, detail);

	fault_page = mmap(NULL, (size_t) getpagesize(), PROT_NONE, MAP_ANON | MAP_PRIVATE, -1, 0);
	memset(&in_handler, 0xa5, sizeof(in_handler));
	install_querying(SIGSEGV, querying_fault_handler, 0);
	install_querying(SIGBUS, querying_fault_handler, 0);
	*(volatile char*) fault_page = 1;
	signal(SIGSEGV, SIG_DFL);
	signal(SIGBUS, SIG_DFL);
	munmap(fault_page, (size_t) getpagesize());
	describe(detail, sizeof(detail), &in_handler);
	snprintf(what, sizeof(what), "%s: in a SIGSEGV handler without SA_ONSTACK, the alternate stack stays installed",
		thread);
	check(is_altstack(&in_handler, 0), what, detail);

	mine.ss_flags = SS_DISABLE;
	sigaltstack(&mine, NULL);
	signal(SIGUSR1, SIG_DFL);
	return NULL;
}

int main(void)
{
	setvbuf(stdout, NULL, _IOLBF, 0);

	test_ignored_by_default();
	test_flags();
	test_resethand(SIGUSR2);
	test_resethand(SIGWINCH);
	test_resethand(SIGURG);
	test_resethand(SIGCHLD);

	pthread_t thread;
	pthread_create(&thread, NULL, altstack_reports, "new thread");
	pthread_join(thread, NULL);
	altstack_reports("main thread");

	if (failures) {
		printf("%d failure(s)\n", failures);
		return 1;
	}
	printf("PASS\n");
	return 0;
}
