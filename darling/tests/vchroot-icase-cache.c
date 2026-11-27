/**
 * This file is part of Darling.
 *
 * Copyright (C) 2026 Lubos Dolezel
 *
 * Darling is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Darling is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Darling.  If not, see <http://www.gnu.org/licenses/>.
 */

// Guest test of case-insensitive path lookup (see vchroot-icase-cache.md): repeated lookups must follow
// every change to the directory, however recent.

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>

static int failures;
static char dir[256];

#define CHECK(cond, ...) do { if (!(cond)) { printf("FAIL line %d: ", __LINE__); printf(__VA_ARGS__); printf("\n"); failures++; } } while (0)

static void path(char* out, const char* name)
{
	snprintf(out, 512, "%s/%s", dir, name);
}

static void touch(const char* name)
{
	char p[512];
	path(p, name);
	int fd = open(p, O_CREAT | O_WRONLY | O_EXCL, 0644);
	if (fd < 0)
	{
		printf("FAIL cannot create %s: %s\n", p, strerror(errno));
		failures++;
		return;
	}
	close(fd);
}

static void remove_name(const char* name)
{
	char p[512];
	path(p, name);
	if (unlink(p))
	{
		printf("FAIL cannot unlink %s: %s\n", p, strerror(errno));
		failures++;
	}
}

// Backdates the directory's modification time, so that a lookup result is eligible for caching
static void age_directory(void)
{
	struct timeval tv[2];
	gettimeofday(&tv[0], NULL);
	tv[0].tv_sec -= 3600;
	tv[1] = tv[0];
	if (utimes(dir, tv))
	{
		printf("FAIL utimes: %s\n", strerror(errno));
		failures++;
	}
}

static ino_t inode_of(const char* name, int* err)
{
	char p[512];
	struct stat st;
	path(p, name);
	if (stat(p, &st))
	{
		*err = errno;
		return 0;
	}
	*err = 0;
	return st.st_ino;
}

// Looks `name` up (as given, so a different case needs the case-insensitive fallback) several times
static void expect_found(const char* name, const char* real)
{
	int err, err2;
	ino_t want = inode_of(real, &err);
	CHECK(err == 0, "%s does not exist", real);
	for (int i = 0; i < 4; i++)
	{
		ino_t got = inode_of(name, &err2);
		CHECK(err2 == 0 && got == want, "lookup of %s (round %d): errno %d, inode %llu, want %llu", name, i, err2, (unsigned long long) got, (unsigned long long) want);
	}
}

static void expect_missing(const char* name)
{
	for (int i = 0; i < 4; i++)
	{
		int err;
		inode_of(name, &err);
		CHECK(err == ENOENT, "lookup of %s (round %d): errno %d, want ENOENT", name, i, err);
	}
}

static volatile int stop;
static volatile int thread_failures;

static void* hammer(void* unused)
{
	(void) unused;
	while (!stop)
	{
		int err;
		inode_of("STABLE.TXT", &err);
		if (err)
			thread_failures++;
		inode_of("Stable.txt", &err);
		if (err)
			thread_failures++;
	}
	return NULL;
}

int main(void)
{
	snprintf(dir, sizeof(dir), "/tmp/icase-cache-XXXXXX");
	if (!mkdtemp(dir))
	{
		perror("mkdtemp");
		return 2;
	}

	touch("Stable.txt");
	touch("MixedCase.Dat");
	age_directory();

	// Another case finds the stored name, again and again
	expect_found("mixedcase.dat", "MixedCase.Dat");
	expect_found("MIXEDCASE.DAT", "MixedCase.Dat");

	// A name that does not exist stays missing, including once the directory is old enough for the answer to be kept
	expect_missing("absent.txt");
	expect_missing("ABSENT.TXT");

	// ... and is found as soon as it is created, in whatever case
	touch("Absent.Txt");
	expect_found("absent.txt", "Absent.Txt");
	expect_found("ABSENT.TXT", "Absent.Txt");

	// Renaming away from the name makes the old spelling missing and the new one found
	{
		char from[512], to[512];
		path(from, "MixedCase.Dat");
		path(to, "Renamed.Dat");
		CHECK(rename(from, to) == 0, "rename: %s", strerror(errno));
	}
	expect_missing("mixedcase.dat");
	expect_found("RENAMED.DAT", "Renamed.Dat");

	// Replacing a name by another spelling of the same name in quick succession, several times over
	for (int i = 0; i < 20; i++)
	{
		const char* spellings[2] = {"Cycle.Bin", "CYCLE.bin"};
		const char* next = spellings[i & 1];
		if (i)
			remove_name(spellings[(i - 1) & 1]);
		touch(next);
		expect_found("cycle.bin", next);
	}
	remove_name("CYCLE.bin");
	expect_missing("cycle.bin");

	// Aged directory, then a name removed and the lookup repeated
	age_directory();
	expect_found("ABSENT.txt", "Absent.Txt");
	remove_name("Absent.Txt");
	expect_missing("absent.txt");
	age_directory();
	expect_missing("absent.txt");
	touch("ABSENT.TXT");
	expect_found("absent.txt", "ABSENT.TXT");

	// A canonically equivalent (decomposed) spelling finds a precomposed name
	touch("\xc3\xbc" "ber.txt");
	age_directory();
	expect_found("u\xcc\x88" "ber.txt", "\xc3\xbc" "ber.txt");
	expect_found("U\xcc\x88" "BER.TXT", "\xc3\xbc" "ber.txt");

	// More names than a cache can hold, each looked up in another case, twice
	for (int i = 0; i < 3000; i++)
	{
		char name[64];
		snprintf(name, sizeof(name), "Many-%04d.Dat", i);
		touch(name);
	}
	age_directory();
	for (int round = 0; round < 2; round++)
	{
		for (int i = 0; i < 3000; i++)
		{
			char name[64], other[64];
			int err;
			snprintf(name, sizeof(name), "Many-%04d.Dat", i);
			snprintf(other, sizeof(other), "MANY-%04d.dat", i);
			ino_t want = inode_of(name, &err);
			ino_t got = inode_of(other, &err);
			if (err || got != want)
			{
				printf("FAIL line %d: lookup of %s, round %d: errno %d\n", __LINE__, other, round, err);
				failures++;
				break;
			}
		}
	}

	// A forked child sees the same, including changes the parent makes afterwards and vice versa
	{
		pid_t pid = fork();
		if (pid == 0)
		{
			int err;
			ino_t a = inode_of("STABLE.txt", &err);
			if (err || !a)
				_exit(10);
			touch("ChildMade.Txt");
			inode_of("childmade.txt", &err);
			_exit(err ? 11 : failures ? 12 : 0);
		}
		int status = 0;
		waitpid(pid, &status, 0);
		CHECK(WIFEXITED(status) && WEXITSTATUS(status) == 0, "child status %d", status);
		expect_found("CHILDMADE.TXT", "ChildMade.Txt");
	}

	// Threads look a stable name up while another thread changes the directory
	{
		pthread_t threads[3];
		for (int i = 0; i < 3; i++)
			pthread_create(&threads[i], NULL, hammer, NULL);
		for (int i = 0; i < 200; i++)
		{
			char name[64];
			snprintf(name, sizeof(name), "Churn-%d.Tmp", i & 7);
			touch(name);
			{
			char upper[64];
			for (size_t j = 0; (upper[j] = name[j] >= 'a' && name[j] <= 'z' ? name[j] - 32 : name[j]); j++);
			expect_found(upper, name);
		}
			remove_name(name);
			expect_missing(name);
		}
		stop = 1;
		for (int i = 0; i < 3; i++)
			pthread_join(threads[i], NULL);
		CHECK(thread_failures == 0, "%d failed lookups in other threads", thread_failures);
	}

	// Clean up
	{
		char cmd[400];
		snprintf(cmd, sizeof(cmd), "rm -rf '%s'", dir);
		if (system(cmd))
			printf("note: could not remove %s\n", dir);
	}

	printf("%s vchroot icase cache\n", failures ? "FAIL" : "PASS");
	return failures ? 1 : 0;
}
