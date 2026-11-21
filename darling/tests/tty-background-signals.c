// Copyright (C) 2026 Lubos Dolezel
// What a background process group gets from its controlling terminal depends on whether it
// ignores or blocks the job-control signals (POSIX General Terminal Interface, tcsetpgrp(),
// termios(4)):
// - tcsetpgrp() succeeds when SIGTTOU is ignored or blocked; otherwise the group is sent
//   SIGTTOU, which stops it by default. Job-control shells rely on this: the forked child of
//   zsh moves itself into its own process group and calls tcsetpgrp() with SIGTTOU ignored.
// - read() fails with EIO when SIGTTIN is ignored or blocked; otherwise the group is sent
//   SIGTTIN, which stops it by default.
// The EIO is not to be confused with the end of file that read() returns on either side of a
// pseudo-terminal once the other side is closed (observed on macOS 26.7.3), which is checked too.
//
// Each case runs in a new session on a new pty: the session leader takes the pty as its
// controlling terminal, sets the signal as the case says, and forks a child that inherits it,
// moves into its own process group (now in the background, and not orphaned, as its parent is
// in another group of the session) and makes the call. An alarm turns a hang into a failure.
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

enum disposition { IGNORED_SIGACTION, IGNORED_SIGNAL, IGNORED_IN_CHILD, BLOCKED, DEFAULT };
enum op { SET_FOREGROUND, READ };

static const char* const disposition_names[] = { "SIG_IGN via sigaction()", "SIG_IGN via signal()",
	"SIG_IGN set in the child", "blocked", "default" };
static const char* const op_names[] = { "tcsetpgrp()", "read()" };
static const int op_signals[] = { SIGTTOU, SIGTTIN };

static void set_ignored(int sig)
{
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	sigaction(sig, &sa, NULL);
}

// Runs in the background child. Exit status 0: the call behaved as for an ignored or blocked
// signal (tcsetpgrp() made the group the foreground, read() failed with EIO); 2: it did not.
static void child(int tty, enum op op, enum disposition how)
{
	alarm(5);
	if (how == IGNORED_IN_CHILD)
		set_ignored(op_signals[op]);
	if (setpgid(0, 0) != 0)
		_exit(4);

	if (op == SET_FOREGROUND) {
		if (tcsetpgrp(tty, getpid()) != 0)
			_exit(2);
		_exit(tcgetpgrp(tty) == getpid() ? 0 : 2);
	} else {
		char c;
		ssize_t r = read(tty, &c, 1);
		_exit(r == -1 && errno == EIO ? 0 : 2);
	}
}

// Runs as the session leader; returns the case's result as an exit status: 0 pass, 1 fail.
static int leader(const char* slave_name, enum op op, enum disposition how)
{
	const char* what = disposition_names[how];
	int sig = op_signals[op];

	if (setsid() < 0)
		return 1;
	int tty = open(slave_name, O_RDWR);
	if (tty < 0 || ioctl(tty, TIOCSCTTY, 0) != 0) {
		printf("FAIL: %s, %s: cannot take the pty as controlling terminal: %s\n", op_names[op], what,
			strerror(errno));
		return 1;
	}

	switch (how) {
	case IGNORED_SIGACTION:
		set_ignored(sig);
		break;
	case IGNORED_SIGNAL:
		signal(sig, SIG_IGN);
		break;
	case BLOCKED: {
		sigset_t set;
		sigemptyset(&set);
		sigaddset(&set, sig);
		sigprocmask(SIG_BLOCK, &set, NULL);
		break;
	}
	case IGNORED_IN_CHILD:
	case DEFAULT:
		break;
	}

	pid_t pid = fork();
	if (pid < 0)
		return 1;
	if (pid == 0)
		child(tty, op, how);

	int status;
	if (waitpid(pid, &status, WUNTRACED) != pid) {
		printf("FAIL: %s, %s: waitpid: %s\n", op_names[op], what, strerror(errno));
		return 1;
	}

	if (how == DEFAULT) {
		// The default action of the signal stops the background group instead.
		if (WIFSTOPPED(status) && WSTOPSIG(status) == sig) {
			kill(pid, SIGKILL);
			waitpid(pid, &status, 0);
			return 0;
		}
		printf("FAIL: %s, %s: child was not stopped by signal %d (status 0x%x)\n", op_names[op], what,
			sig, status);
		if (WIFSTOPPED(status))
			kill(pid, SIGKILL);
		return 1;
	}

	if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
		return 0;
	if (WIFSIGNALED(status) && WTERMSIG(status) == SIGALRM)
		printf("FAIL: %s, %s: the call in the background group hung\n", op_names[op], what);
	else if (WIFSTOPPED(status)) {
		printf("FAIL: %s, %s: child was stopped by signal %d\n", op_names[op], what, WSTOPSIG(status));
		kill(pid, SIGKILL);
	} else if (WIFEXITED(status) && WEXITSTATUS(status) == 2)
		printf("FAIL: %s, %s: %s\n", op_names[op], what, op == SET_FOREGROUND
			? "tcsetpgrp() failed or did not change the foreground group" : "read() did not fail with EIO");
	else
		printf("FAIL: %s, %s: child status 0x%x\n", op_names[op], what, status);
	return 1;
}

// read() on one side of a pseudo-terminal whose other side is closed returns 0.
static int check_closed_side(bool close_master)
{
	const char* what = close_master ? "read() on the slave after the master is closed"
		: "read() on the master after the slave is closed";
	int master = posix_openpt(O_RDWR | O_NOCTTY);
	if (master < 0 || grantpt(master) != 0 || unlockpt(master) != 0) {
		printf("FAIL: cannot open a pty: %s\n", strerror(errno));
		return 1;
	}
	int slave = open(ptsname(master), O_RDWR | O_NOCTTY);
	if (slave < 0) {
		printf("FAIL: cannot open the pty slave: %s\n", strerror(errno));
		return 1;
	}
	close(close_master ? master : slave);

	char c;
	errno = 0;
	ssize_t r = read(close_master ? slave : master, &c, 1);
	int err = errno;
	close(close_master ? slave : master);
	if (r != 0) {
		printf("FAIL: %s returned %ld, errno %d; expected 0\n", what, (long) r, err);
		return 1;
	}
	printf("ok: %s\n", what);
	return 0;
}

int main(void)
{
	int failures = 0;

	// The session leaders report with printf() and leave with _exit().
	setvbuf(stdout, NULL, _IONBF, 0);

	failures += check_closed_side(false);
	failures += check_closed_side(true);

	for (int op = SET_FOREGROUND; op <= READ; op++) {
		for (int how = IGNORED_SIGACTION; how <= DEFAULT; how++) {
			int master = posix_openpt(O_RDWR | O_NOCTTY);
			if (master < 0 || grantpt(master) != 0 || unlockpt(master) != 0) {
				printf("FAIL: cannot open a pty: %s\n", strerror(errno));
				return 1;
			}
			char slave_name[128];
			strncpy(slave_name, ptsname(master), sizeof(slave_name) - 1);
			slave_name[sizeof(slave_name) - 1] = '\0';

			pid_t pid = fork();
			if (pid == 0)
				_exit(leader(slave_name, op, how));

			int status = 0;
			if (pid < 0 || waitpid(pid, &status, 0) != pid || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
				if (pid > 0 && !WIFEXITED(status))
					printf("FAIL: %s, %s: session leader status 0x%x\n", op_names[op],
						disposition_names[how], status);
				failures++;
			} else
				printf("ok: %s, %s\n", op_names[op], disposition_names[how]);
			close(master);
		}
	}

	if (failures) {
		printf("FAIL: %d case(s)\n", failures);
		return 1;
	}
	printf("PASS\n");
	return 0;
}
