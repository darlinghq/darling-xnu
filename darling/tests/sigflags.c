// sigaction(2) flags that change how a handler is run:
// - SA_NODEFER: the signal is not blocked while its handler runs, so the handler is entered again when the
//   signal arrives during it; without the flag the second delivery waits until the handler returns.
// - SA_RESETHAND: the disposition goes back to SIG_DFL when the handler is entered (except for SIGILL and
//   SIGTRAP), so the next delivery takes the default action.
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

int main(void)
{
	setvbuf(stdout, NULL, _IOLBF, 0);

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
