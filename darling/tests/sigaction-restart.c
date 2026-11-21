// Copyright (C) 2026 Lubos Dolezel
// A blocking system call that a signal handler interrupts returns -1/EINTR unless the handler was installed
// with SA_RESTART, in which case the call continues (sigaction(2)). signal() installs handlers with SA_RESTART.
// A signal that is ignored, or is ignored by default (SIGCHLD, SIGWINCH, SIGURG), never interrupts a call.
// select() and nanosleep() are never restarted, whatever SA_RESTART says (observed on macOS 26.7.3 and 10.14.4).
//
// Each case blocks the process in a call (read on a pipe, a pty master and a pty slave; accept; wait4; select;
// nanosleep) while a helper process sends the signal after 200 ms and completes the call (writes, connects, exits)
// after the delay of the case. If the call is expected to be interrupted, that delay is long, so a restarting
// call shows up as a call that took its full delay and succeeded, not as a hang.
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

enum install { NORESTART, RESTART, VIA_SIGNAL, IGNORE, DEFAULT };
enum call { READ_PIPE, READ_PTY_MASTER, READ_PTY_SLAVE, ACCEPT, WAIT4, SELECT, NANOSLEEP };

static const char* const install_names[] = { "no SA_RESTART", "SA_RESTART", "signal()", "SIG_IGN", "default" };
static const char* const call_names[] = { "read(pipe)", "read(pty master)", "read(pty slave)", "accept", "wait4",
	"select", "nanosleep" };

static volatile sig_atomic_t handler_runs;
static int failures;

static void handler(int sig)
{
	(void) sig;
	handler_runs++;
}

static void sleep_ms(long ms)
{
	struct timespec ts = { ms / 1000, (ms % 1000) * 1000000L };
	while (nanosleep(&ts, &ts) == -1 && errno == EINTR)
		;
}

static void install_handler(int sig, enum install how)
{
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sigemptyset(&sa.sa_mask);
	switch (how) {
	case NORESTART:
	case RESTART:
		sa.sa_handler = handler;
		sa.sa_flags = how == RESTART ? SA_RESTART : 0;
		break;
	case VIA_SIGNAL:
		if (signal(sig, handler) == SIG_ERR) {
			perror("signal");
			exit(2);
		}
		return;
	case IGNORE:
		sa.sa_handler = SIG_IGN;
		break;
	case DEFAULT:
		sa.sa_handler = SIG_DFL;
		break;
	}
	if (sigaction(sig, &sa, NULL) != 0) {
		perror("sigaction");
		exit(2);
	}
}

static void check(int ok, const char* what, const char* detail)
{
	if (!ok) {
		printf("FAIL: %s: %s\n", what, detail);
		failures++;
	} else {
		printf("ok: %s\n", what);
	}
}

// expect_eintr: the call must fail with EINTR after the handler ran; otherwise it must complete normally
// and the handler must have run exactly when expect_handler says.
static void run_case(enum call call, int sig, enum install how, int expect_eintr, int expect_handler)
{
	char what[128], detail[160];
	snprintf(what, sizeof(what), "%s, %s, signal %d", call_names[call], install_names[how], sig);

	int pipefd[2] = { -1, -1 }, master = -1, slave = -1, listener = -1, wait_fd = -1;
	struct sockaddr_un addr;
	char path[] = "/tmp/sigaction-restart.XXXXXX";
	// the helper's completion after this many ms (long if the call should be interrupted)
	long delay = expect_eintr ? 3000 : 700;

	switch (call) {
	case READ_PIPE:
	case SELECT:
		if (pipe(pipefd) != 0) {
			perror("pipe");
			exit(2);
		}
		wait_fd = pipefd[0];
		break;
	case READ_PTY_MASTER:
	case READ_PTY_SLAVE:
		master = posix_openpt(O_RDWR | O_NOCTTY);
		if (master < 0 || grantpt(master) != 0 || unlockpt(master) != 0) {
			perror("pty");
			exit(2);
		}
		slave = open(ptsname(master), O_RDWR | O_NOCTTY);
		if (slave < 0) {
			perror("open slave");
			exit(2);
		}
		wait_fd = call == READ_PTY_MASTER ? master : slave;
		break;
	case ACCEPT:
		{
			// a unique name for the socket: make the file, then remove it so that bind() can create it
			int tmp = mkstemp(path);
			if (tmp < 0) {
				perror("mkstemp");
				exit(2);
			}
			close(tmp);
			unlink(path);
		}
		listener = socket(AF_UNIX, SOCK_STREAM, 0);
		memset(&addr, 0, sizeof(addr));
		addr.sun_family = AF_UNIX;
		strcpy(addr.sun_path, path);
		if (listener < 0 || bind(listener, (struct sockaddr*) &addr, sizeof(addr)) != 0 || listen(listener, 1) != 0) {
			perror("listener");
			exit(2);
		}
		break;
	default:
		break;
	}

	handler_runs = 0;
	install_handler(sig, how);

	pid_t parent = getpid();
	pid_t helper = fork();
	if (helper < 0) {
		perror("fork");
		exit(2);
	}
	if (helper == 0) {
		sleep_ms(200);
		kill(parent, sig);
		sleep_ms(delay - 200);
		switch (call) {
		case READ_PIPE:
		case SELECT:
			write(pipefd[1], "data", 4);
			break;
		case READ_PTY_MASTER:
			write(slave, "data\n", 5);
			break;
		case READ_PTY_SLAVE:
			write(master, "data\n", 5);
			break;
		case ACCEPT: {
			int c = socket(AF_UNIX, SOCK_STREAM, 0);
			connect(c, (struct sockaddr*) &addr, sizeof(addr));
			break;
		}
		default:
			break;
		}
		_exit(0);
	}

	struct timespec t0, t1;
	clock_gettime(CLOCK_MONOTONIC, &t0);
	long rc = 0;
	int err = 0;
	char buf[16];
	int status;
	switch (call) {
	case READ_PIPE:
	case READ_PTY_MASTER:
	case READ_PTY_SLAVE:
		rc = read(wait_fd, buf, sizeof(buf));
		break;
	case ACCEPT:
		rc = accept(listener, NULL, NULL);
		if (rc >= 0)
			close((int) rc);
		break;
	case WAIT4:
		rc = wait4(helper, &status, 0, NULL);
		break;
	case SELECT: {
		fd_set set;
		FD_ZERO(&set);
		FD_SET(wait_fd, &set);
		rc = select(wait_fd + 1, &set, NULL, NULL, NULL);
		break;
	}
	case NANOSLEEP: {
		struct timespec ts = { 3, 0 };
		rc = nanosleep(&ts, NULL);
		break;
	}
	}
	err = errno;
	clock_gettime(CLOCK_MONOTONIC, &t1);
	long elapsed_ms = (t1.tv_sec - t0.tv_sec) * 1000 + (t1.tv_nsec - t0.tv_nsec) / 1000000;

	kill(helper, SIGKILL);
	waitpid(helper, NULL, 0);
	if (master >= 0)
		close(master);
	if (slave >= 0)
		close(slave);
	if (pipefd[0] >= 0) {
		close(pipefd[0]);
		close(pipefd[1]);
	}
	if (listener >= 0) {
		close(listener);
		unlink(path);
	}

	// leave the signal at its default for the next case
	install_handler(sig, DEFAULT);

	if (expect_eintr) {
		snprintf(detail, sizeof(detail), "returned %ld, errno %d after %ld ms, handler ran %d times; expected -1/EINTR",
			rc, err, elapsed_ms, (int) handler_runs);
		check(rc == -1 && err == EINTR && handler_runs == 1 && elapsed_ms < 2000, what, detail);
	} else {
		snprintf(detail, sizeof(detail), "returned %ld, errno %d after %ld ms, handler ran %d times; expected success"
			" after the helper completed it, handler ran %d times",
			rc, err, elapsed_ms, (int) handler_runs, expect_handler);
		int completed = call == NANOSLEEP ? rc == 0 : rc >= 0;
		check(completed && handler_runs == expect_handler && elapsed_ms >= (call == NANOSLEEP ? 2900 : 600), what,
			detail);
	}
}

static pthread_mutex_t cancel_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cancel_cond = PTHREAD_COND_INITIALIZER;

static void* cancel_cond_waiter(void* arg)
{
	(void) arg;
	pthread_mutex_lock(&cancel_mutex);
	for (;;)
		pthread_cond_wait(&cancel_cond, &cancel_mutex); // only the cancellation ends this
	return NULL;
}

static void* cancel_sleeper(void* arg)
{
	(void) arg;
	for (;;)
		sleep(100); // only the cancellation ends this
	return NULL;
}

// pthread_cancel() ends a thread blocked in a cancellation point even when the application's handlers
// for every signal are installed without SA_RESTART (cancellation does not go through the application's
// signal handlers, so those flags must not change how it interrupts the thread)
static void run_cancel_case(const char* what, void* (*body)(void*))
{
	for (int sig = 1; sig < 32; sig++) {
		if (sig == SIGKILL || sig == SIGSTOP || sig == SIGPIPE || sig == SIGCHLD)
			continue;
		install_handler(sig, NORESTART);
	}
	pthread_t th;
	pthread_create(&th, NULL, body, NULL);
	sleep_ms(300); // let the thread block
	struct timespec t0, t1;
	clock_gettime(CLOCK_MONOTONIC, &t0);
	pthread_cancel(th);
	void* result = NULL;
	pthread_join(th, &result);
	clock_gettime(CLOCK_MONOTONIC, &t1);
	long elapsed_ms = (t1.tv_sec - t0.tv_sec) * 1000 + (t1.tv_nsec - t0.tv_nsec) / 1000000;
	check(result == PTHREAD_CANCELED && elapsed_ms < 2000, what, "the thread was not cancelled while blocked");
	for (int sig = 1; sig < 32; sig++) {
		if (sig == SIGKILL || sig == SIGSTOP || sig == SIGPIPE || sig == SIGCHLD)
			continue;
		install_handler(sig, DEFAULT);
	}
}

int main(void)
{
	setvbuf(stdout, NULL, _IOLBF, 0);

	// the helper writes to descriptors whose reader may be gone
	signal(SIGPIPE, SIG_IGN);

	enum call blocking[] = { READ_PIPE, READ_PTY_MASTER, READ_PTY_SLAVE, ACCEPT, WAIT4 };
	for (unsigned i = 0; i < sizeof(blocking) / sizeof(blocking[0]); i++) {
		enum call c = blocking[i];
		// no SA_RESTART: the handler runs and the call fails with EINTR
		run_case(c, SIGUSR1, NORESTART, 1, 1);
		// SA_RESTART, and signal(), which implies it: the call continues until it completes
		run_case(c, SIGUSR1, RESTART, 0, 1);
		run_case(c, SIGUSR1, VIA_SIGNAL, 0, 1);
		// an ignored signal does nothing
		run_case(c, SIGUSR1, IGNORE, 0, 0);
	}

	// the symptom: SIGALRM from the real-time timer, as a program with an alarm handler sees it
	{
		struct sigaction sa;
		memset(&sa, 0, sizeof(sa));
		sa.sa_handler = handler;
		sigaction(SIGALRM, &sa, NULL);
		int p[2];
		pipe(p);
		struct itimerval it = { { 0, 0 }, { 0, 200000 } };
		handler_runs = 0;
		setitimer(ITIMER_REAL, &it, NULL);
		// only the interruption should end the read; a helper writes after 3 s so that a read that restarts fails
		// the check instead of blocking for ever
		pid_t writer = fork();
		if (writer == 0) {
			sleep_ms(3000);
			write(p[1], "x", 1);
			_exit(0);
		}
		char b;
		ssize_t rc = read(p[0], &b, 1);
		int err = errno;
		kill(writer, SIGKILL);
		waitpid(writer, NULL, 0);
		check(rc == -1 && err == EINTR && handler_runs == 1, "read(pipe), setitimer SIGALRM, no SA_RESTART",
			"read was not interrupted with EINTR");
		signal(SIGALRM, SIG_DFL);
		close(p[0]);
		close(p[1]);
	}

	// signals that are ignored by default never interrupt a call
	int ignored_by_default[] = { SIGCHLD, SIGWINCH, SIGURG };
	for (unsigned i = 0; i < sizeof(ignored_by_default) / sizeof(ignored_by_default[0]); i++) {
		run_case(READ_PIPE, ignored_by_default[i], DEFAULT, 0, 0);
		run_case(READ_PTY_MASTER, ignored_by_default[i], DEFAULT, 0, 0);
	}

	// the flag follows the latest sigaction(): a handler reinstalled with or without SA_RESTART changes it
	run_case(READ_PIPE, SIGUSR2, RESTART, 0, 1);
	run_case(READ_PIPE, SIGUSR2, NORESTART, 1, 1);

	// select() and nanosleep() are not restarted even with SA_RESTART
	run_case(SELECT, SIGUSR1, RESTART, 1, 1);
	run_case(NANOSLEEP, SIGUSR1, RESTART, 1, 1);
	run_case(SELECT, SIGUSR1, NORESTART, 1, 1);
	run_case(NANOSLEEP, SIGUSR1, NORESTART, 1, 1);

	run_cancel_case("pthread_cancel of a thread blocked in pthread_cond_wait()", cancel_cond_waiter);
	run_cancel_case("pthread_cancel of a thread blocked in sleep()", cancel_sleeper);

	// sigaction(2): the flags read back are the ones set, and signal() sets SA_RESTART
	{
		struct sigaction old;
		install_handler(SIGUSR1, VIA_SIGNAL);
		sigaction(SIGUSR1, NULL, &old);
		check((old.sa_flags & SA_RESTART) != 0, "signal() flags", "SA_RESTART not set on a handler set with signal()");
		install_handler(SIGUSR1, NORESTART);
		sigaction(SIGUSR1, NULL, &old);
		check((old.sa_flags & SA_RESTART) == 0, "sigaction() flags", "SA_RESTART reads back set");
	}

	if (failures) {
		printf("%d failure(s)\n", failures);
		return 1;
	}
	printf("PASS\n");
	return 0;
}
