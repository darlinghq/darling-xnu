// What sigaction(2) and sigaltstack(2) report back, following XNU (bsd/kern/kern_sig.c):
// - sigaction() reports the flags XNU keeps for a signal: SA_ONSTACK, SA_RESTART, SA_SIGINFO, SA_NODEFER,
//   and for SIGCHLD only SA_NOCLDSTOP and SA_NOCLDWAIT (also reported after SIGCHLD was set to SIG_IGN,
//   which makes children be reaped as with SA_NOCLDWAIT). Never SA_RESETHAND. A signal never set reports
//   SA_RESTART: XNU records which signals interrupt system calls, and none does until an action without
//   SA_RESTART is set. The mask is reported without SIGKILL and SIGSTOP, which cannot be blocked.
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

int main(void)
{
	setvbuf(stdout, NULL, _IOLBF, 0);

	test_flags();

	if (failures) {
		printf("%d failure(s)\n", failures);
		return 1;
	}
	printf("PASS\n");
	return 0;
}
