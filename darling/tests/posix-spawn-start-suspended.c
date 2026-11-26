// Copyright (C) 2026 Lubos Dolezel
//
// posix_spawn() with POSIX_SPAWN_START_SUSPENDED creates the child stopped: the parent sees it with
// waitpid(WUNTRACED), the child does not run until the parent sends it SIGCONT, and kill(SIGSTOP) stops a
// running process the same way (WIFSTOPPED, WSTOPSIG == SIGSTOP) until SIGCONT.
#include <errno.h>
#include <signal.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

extern char **environ;

static int failures;

#define CHECK(cond, msg) do { if (!(cond)) { printf("FAIL: %s\n", msg); failures++; } } while (0)

// waitpid() that gives up after about five seconds instead of blocking the test forever
static pid_t wait_for(pid_t pid, int *status, int options)
{
	for (int i = 0; i < 500; i++) {
		pid_t r = waitpid(pid, status, options | WNOHANG);
		if (r != 0)
			return r;
		usleep(10000);
	}
	return 0;
}

int main(int argc, char **argv)
{
	setvbuf(stdout, NULL, _IONBF, 0);
	if (argc > 1 && strcmp(argv[1], "child") == 0)
		_exit(7);

	posix_spawnattr_t attr;
	posix_spawnattr_init(&attr);
	CHECK(posix_spawnattr_setflags(&attr, POSIX_SPAWN_START_SUSPENDED) == 0, "setflags");
	pid_t pid;
	char *args[] = { argv[0], "child", NULL };
	int rc = posix_spawn(&pid, argv[0], NULL, &attr, args, environ);
	if (rc != 0) {
		printf("FAIL: posix_spawn: %s\n", strerror(rc));
		return 1;
	}

	int st = 0;
	pid_t r = wait_for(pid, &st, WUNTRACED);
	CHECK(r == pid, "waitpid(WUNTRACED) reports the suspended child");
	CHECK(r == pid && WIFSTOPPED(st), "the suspended child's status is a stop status");
	CHECK(r == pid && WIFSTOPPED(st) && WSTOPSIG(st) == 0, "the suspended child is stopped without a signal, as on macOS");

	// It must stay stopped: it neither exits nor reports anything else.
	usleep(300000);
	r = waitpid(pid, &st, WNOHANG);
	CHECK(r == 0, "the suspended child does not run before SIGCONT");

	CHECK(kill(pid, SIGCONT) == 0, "kill(SIGCONT)");
	r = wait_for(pid, &st, 0);
	CHECK(r == pid && WIFEXITED(st) && WEXITSTATUS(st) == 7, "the child runs and exits with 7 after SIGCONT");

	// kill(SIGSTOP) of a running process
	pid = fork();
	if (pid == 0) {
		sleep(30);
		_exit(3);
	}
	usleep(200000);
	CHECK(kill(pid, SIGSTOP) == 0, "kill(SIGSTOP)");
	r = wait_for(pid, &st, WUNTRACED);
	CHECK(r == pid && WIFSTOPPED(st) && WSTOPSIG(st) == SIGSTOP, "kill(SIGSTOP) is reported as stopped by SIGSTOP");
	kill(pid, SIGCONT);
	kill(pid, SIGKILL);
	r = wait_for(pid, &st, 0);
	CHECK(r == pid && WIFSIGNALED(st) && WTERMSIG(st) == SIGKILL, "a stopped-then-continued process can be killed");

	if (failures == 0)
		printf("PASS: POSIX_SPAWN_START_SUSPENDED and SIGSTOP\n");
	return failures != 0;
}
