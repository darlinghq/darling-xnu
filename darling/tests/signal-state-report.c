// What sigaction(2) and sigaltstack(2) report back, following XNU (bsd/kern/kern_sig.c):
// - sigaction() reports the flags XNU keeps for a signal: SA_ONSTACK, SA_RESTART, SA_SIGINFO, SA_NODEFER,
//   and for SIGCHLD only SA_NOCLDSTOP and SA_NOCLDWAIT (also reported after SIGCHLD was set to SIG_IGN,
//   which makes children be reaped as with SA_NOCLDWAIT). Never SA_RESETHAND. A signal never set reports
//   no flags at all, SA_RESTART included (observed on macOS 26.7.3 and 10.14.4; on 26.7.3 a signal that the
//   parent process set with signal() before exec also reports SA_RESTART, which this test cannot depend on,
//   so it looks at signals no shell sets). The mask is reported without SIGKILL and SIGSTOP, which cannot
//   be blocked.
// - after SA_RESETHAND took effect, the action is SIG_DFL without SA_SIGINFO and SA_NODEFER, also for the
//   signals whose default action is to ignore them (SIGCHLD, SIGURG, SIGWINCH), which are not SIG_IGN.
//   Those, and SIGIO and SIGINFO, are ignored by default: raising them does not end the process.
// - sigaltstack() reports the alternate stack the thread installed, SS_DISABLE with no address and size
//   when it installed none, also inside a handler; and the installed one, also while a handler without
//   SA_ONSTACK runs on the interrupted stack.
// - a sigaltstack() call whose new stack is rejected: EINVAL for flags other than SS_DISABLE, which still
//   fills in the old stack, and ENOMEM for a stack that is too small, which leaves it alone.
// - a SIGCHLD handler without SA_RESTART interrupts a blocking call, which fails with EINTR.
#include <errno.h>
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
	check_action(SIGXCPU, (void*) SIG_DFL, 0, "a signal never set reports SIG_DFL with no flags");

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

// --- flags the reports keep, observed on macOS 26.7.3 and 10.14.4

static void test_flag_bookkeeping(void)
{
	struct sigaction act;
	memset(&act, 0, sizeof(act));

	// signal() installs with SA_RESTART
	signal(SIGUSR2, SIG_DFL);
	check_action(SIGUSR2, (void*) SIG_DFL, SA_RESTART, "signal(SIG_DFL) reports SA_RESTART");
	signal(SIGUSR2, SIG_IGN);
	check_action(SIGUSR2, (void*) SIG_IGN, SA_RESTART, "signal(SIG_IGN) reports SA_RESTART");

	// the flags given with SIG_DFL and SIG_IGN are kept
	act.sa_handler = SIG_DFL;
	act.sa_flags = SA_RESTART;
	sigaction(SIGUSR2, &act, NULL);
	check_action(SIGUSR2, (void*) SIG_DFL, SA_RESTART, "SIG_DFL set with SA_RESTART reports SA_RESTART");
	act.sa_handler = SIG_IGN;
	act.sa_flags = 0;
	sigaction(SIGUSR2, &act, NULL);
	check_action(SIGUSR2, (void*) SIG_IGN, 0, "SIG_IGN set without flags reports no flags");

	// SIGCHLD: SIG_IGN reports SA_NOCLDWAIT, which does not stay when the action is set again
	act.sa_handler = SIG_IGN;
	act.sa_flags = SA_NOCLDSTOP;
	sigaction(SIGCHLD, &act, NULL);
	check_action(SIGCHLD, (void*) SIG_IGN, SA_NOCLDSTOP | SA_NOCLDWAIT, "SIGCHLD set to SIG_IGN with SA_NOCLDSTOP");
	act.sa_handler = SIG_DFL;
	act.sa_flags = 0;
	sigaction(SIGCHLD, &act, NULL);
	check_action(SIGCHLD, (void*) SIG_DFL, 0, "SIGCHLD set to SIG_DFL again: SA_NOCLDWAIT is gone");

	// what an SA_RESETHAND reset keeps: the other flags the action was set with
	sigset_t none;
	sigemptyset(&none);
	install(SIGUSR2, SA_SIGINFO | SA_RESETHAND | SA_ONSTACK, none);
	raise(SIGUSR2);
	check_action(SIGUSR2, (void*) SIG_DFL, SA_ONSTACK, "SA_RESETHAND reset keeps SA_ONSTACK");
	install(SIGUSR2, SA_SIGINFO | SA_RESETHAND, none);
	raise(SIGUSR2);
	check_action(SIGUSR2, (void*) SIG_DFL, 0, "SA_RESETHAND reset without SA_RESTART reports no flags");
	signal(SIGUSR2, SIG_DFL);
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
		snprintf(what, sizeof(what), "signal %d, ignored by default, reports SIG_DFL with no flags before it is set",
			sigs[i]);
		check_action(sigs[i], (void*) SIG_DFL, 0, what);
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

// --- a rejected sigaltstack() call: observed on macOS 26.7.3 and 10.14.4

static void test_altstack_rejected(void)
{
	char detail[192];
	stack_t mine = { .ss_sp = altstack, .ss_size = sizeof(altstack), .ss_flags = 0 };
	sigaltstack(&mine, NULL);

	// QUIRK: flags that are not SS_DISABLE are rejected with EINVAL, and oss is filled in anyway;
	// too small a stack is rejected with ENOMEM and oss stays as it was. Pinned by the checks below.
	stack_t bad = { .ss_sp = altstack, .ss_size = sizeof(altstack), .ss_flags = 0x40 };
	stack_t old;
	memset(&old, 0xa5, sizeof(old));
	int rc = sigaltstack(&bad, &old);
	int err = errno;
	describe(detail, sizeof(detail), &old);
	check(rc == -1 && err == EINVAL && is_altstack(&old, 0), "an invalid flag is EINVAL and fills in oss", detail);

	stack_t small = { .ss_sp = altstack, .ss_size = 100, .ss_flags = 0 };
	memset(&old, 0xa5, sizeof(old));
	rc = sigaltstack(&small, &old);
	err = errno;
	snprintf(detail, sizeof(detail), "rc %d, errno %d, oss flags %#x", rc, err, old.ss_flags);
	check(rc == -1 && err == ENOMEM && old.ss_flags == (int) 0xa5a5a5a5, "a too small stack is ENOMEM and leaves oss alone",
		detail);

	// the same structure as new and old stack: the old stack is written first, so it is installed again
	stack_t other = { .ss_sp = altstack + 4096, .ss_size = sizeof(altstack) - 4096, .ss_flags = 0 };
	rc = sigaltstack(&other, &other);
	describe(detail, sizeof(detail), &other);
	check(rc == 0 && is_altstack(&other, 0), "the same structure as ss and oss: oss gets the old stack", detail);
	stack_t now;
	sigaltstack(NULL, &now);
	describe(detail, sizeof(detail), &now);
	check(is_altstack(&now, 0), "the same structure as ss and oss: the old stack stays installed", detail);

	mine.ss_flags = SS_DISABLE;
	sigaltstack(&mine, NULL);
}

// --- SIGCHLD, SIGTTOU and SIGTTIN handlers interrupt a blocking call without SA_RESTART

static int unblock_fd;

// ends a read() that the handler failed to interrupt, so that the check fails instead of hanging
static void* write_late(void* arg)
{
	(void) arg;
	sleep(3);
	write(unblock_fd, "x", 1);
	return NULL;
}

static void test_sigchld_interrupts(void)
{
	int fds[2];
	sigset_t none;
	sigemptyset(&none);
	pipe(fds);
	unblock_fd = fds[1];
	// the writer starts with SIGCHLD blocked: the signal is for the reading thread, and taken by the
	// writer it would cut its sleep short
	sigset_t chld, previous;
	sigemptyset(&chld);
	sigaddset(&chld, SIGCHLD);
	pthread_sigmask(SIG_BLOCK, &chld, &previous);
	pthread_t writer;
	pthread_create(&writer, NULL, write_late, NULL);
	pthread_sigmask(SIG_SETMASK, &previous, NULL);
	install(SIGCHLD, SA_SIGINFO, none);
	runs = 0;
	pid_t child = fork();
	if (child == 0) {
		usleep(200000);
		_exit(0);
	}
	char c;
	ssize_t n = read(fds[0], &c, 1);
	int err = errno;
	pthread_join(writer, NULL);
	waitpid(child, NULL, 0);
	signal(SIGCHLD, SIG_DFL);
	close(fds[0]);
	close(fds[1]);
	char detail[96];
	snprintf(detail, sizeof(detail), "read returned %zd, errno %d, handler ran %d times", n, err, (int) runs);
	check(n == -1 && err == EINTR && runs == 1, "a SIGCHLD handler without SA_RESTART interrupts read()", detail);
}

int main(void)
{
	setvbuf(stdout, NULL, _IOLBF, 0);

	test_ignored_by_default();
	test_flags();
	test_flag_bookkeeping();
	test_resethand(SIGUSR2);
	test_resethand(SIGWINCH);
	test_resethand(SIGURG);
	test_resethand(SIGCHLD);

	pthread_t thread;
	pthread_create(&thread, NULL, altstack_reports, "new thread");
	pthread_join(thread, NULL);
	altstack_reports("main thread");
	test_altstack_rejected();

	test_sigchld_interrupts();

	if (failures) {
		printf("%d failure(s)\n", failures);
		return 1;
	}
	printf("PASS\n");
	return 0;
}
