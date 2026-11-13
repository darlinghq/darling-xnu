// Copyright (C) 2026 Lubos Dolezel
// posix_spawn and execve accept a NULL environment pointer, which means an
// empty environment: the child must start and run normally.
#include <stdio.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

static int failures;
#define CHECK(c) do { if (!(c)) { printf("FAIL: %s (line %d)\n", #c, __LINE__); failures++; } } while (0)

int main(void)
{
	char *argv[] = { "/usr/bin/true", NULL };
	pid_t child;
	int st = -1;

	CHECK(posix_spawn(&child, "/usr/bin/true", NULL, NULL, argv, NULL) == 0);
	CHECK(waitpid(child, &st, 0) == child);
	CHECK(WIFEXITED(st) && WEXITSTATUS(st) == 0);

	child = fork();
	if (child == 0) {
		execve("/usr/bin/true", argv, NULL);
		_exit(127);
	}
	CHECK(waitpid(child, &st, 0) == child);
	CHECK(WIFEXITED(st) && WEXITSTATUS(st) == 0);

	printf(failures ? "FAILED\n" : "PASS\n");
	return failures != 0;
}
