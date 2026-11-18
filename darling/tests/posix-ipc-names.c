// Copyright (C) 2026 Lubos Dolezel
// Names of POSIX shared memory objects (shm_open/shm_unlink) and named semaphores (sem_open/sem_unlink) are opaque
// keys on macOS: further slashes are allowed ("/MTX/1a2b3c4d"), "a", "/a" and "//a" are different objects, and the
// length limits are 31 characters for shared memory and 30 for semaphores (ENAMETOOLONG; shm_open("") is EINVAL).
// Expected results observed black-box on macOS 26.7.3 and 10.14.4 (same on both).
// The cases marked QUIRK pin undocumented results: shm_unlink() of a name over the limit gives EINVAL, not ENAMETOOLONG.
#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

static int failures;

#define CHECK(cond)                                                                         \
	do {                                                                                    \
		if (!(cond)) {                                                                      \
			printf("FAIL line %d: %s (errno %d)\n", __LINE__, #cond, errno);                \
			failures++;                                                                     \
		}                                                                                   \
	} while (0)

#define CHECK_ERR(call, err)                                                                \
	do {                                                                                    \
		errno = 0;                                                                          \
		long r_ = (long)(call);                                                             \
		if (r_ != -1 || errno != (err)) {                                                   \
			printf("FAIL line %d: %s: got %ld errno %d, want errno %d\n", __LINE__, #call,  \
			       r_, errno, (err));                                                       \
			failures++;                                                                     \
		}                                                                                   \
	} while (0)

// Names of 30, 31 and 32 characters, the leading slash included.
#define NAME30 "/01234567890123456789012345678"
#define NAME31 "/012345678901234567890123456789"
#define NAME32 "/0123456789012345678901234567890"

static volatile int* map_shm(int fd)
{
	void* p = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	return p == MAP_FAILED ? NULL : (volatile int*)p;
}

// Creates the object, sees it again through a second descriptor and a second mapping, then removes it.
static void shm_roundtrip(const char* name)
{
	shm_unlink(name);
	int fd = shm_open(name, O_CREAT | O_EXCL | O_RDWR, 0600);
	if (fd < 0) {
		printf("FAIL shm_open(\"%s\") create: errno %d\n", name, errno);
		failures++;
		return;
	}
	CHECK(ftruncate(fd, 4096) == 0);
	volatile int* a = map_shm(fd);
	CHECK(a != NULL);
	CHECK_ERR(shm_open(name, O_CREAT | O_EXCL | O_RDWR, 0600), EEXIST);
	int fd2 = shm_open(name, O_RDWR, 0);
	CHECK(fd2 >= 0);
	volatile int* b = fd2 >= 0 ? map_shm(fd2) : NULL;
	CHECK(b != NULL);
	if (a && b) {
		*a = 1234;
		CHECK(*b == 1234);
	}
	CHECK(shm_unlink(name) == 0);
	CHECK_ERR(shm_open(name, O_RDWR, 0), ENOENT);
	CHECK_ERR(shm_unlink(name), ENOENT);
	close(fd);
	if (fd2 >= 0)
		close(fd2);
}

static void sem_roundtrip(const char* name)
{
	sem_unlink(name);
	sem_t* s = sem_open(name, O_CREAT | O_EXCL, 0600, 1);
	if (s == SEM_FAILED) {
		printf("FAIL sem_open(\"%s\") create: errno %d\n", name, errno);
		failures++;
		return;
	}
	errno = 0;
	sem_t* again = sem_open(name, O_CREAT | O_EXCL, 0600, 1);
	CHECK(again == SEM_FAILED && errno == EEXIST);
	sem_t* other = sem_open(name, 0);
	CHECK(other != SEM_FAILED);
	CHECK(sem_wait(s) == 0);
	CHECK(sem_trywait(other) == -1 && errno == EAGAIN);
	CHECK(sem_post(s) == 0);
	CHECK(sem_trywait(other) == 0);
	CHECK(sem_unlink(name) == 0);
	errno = 0;
	CHECK(sem_open(name, 0) == SEM_FAILED && errno == ENOENT);
	sem_close(s);
	if (other != SEM_FAILED)
		sem_close(other);
}

static void test_inner_slash_names(void)
{
	static const char* const names[] = {
		"/MTX/1a2b3c4d", "/BSem/1a2b3c4d", "/Evt/1a2b3c4d", "/a/b/c", "/a/", "/with space", "/dir/./x", "/p%q"
	};
	for (unsigned i = 0; i < sizeof(names) / sizeof(names[0]); i++) {
		shm_roundtrip(names[i]);
		sem_roundtrip(names[i]);
	}
}

// Different names are different objects, including ones the host would fold together.
static void test_names_are_distinct(void)
{
	static const char* const names[] = {
		"/a", "a", "//a", "/a/b", "/a%2Fb", "/a%b", "/", "", "/.", "/..", "/%.a", "%.a"
	};
	enum { N = sizeof(names) / sizeof(names[0]) };
	int fds[N];
	for (int i = 0; i < N; i++) {
		shm_unlink(names[i]);
		fds[i] = -1;
	}
	for (int i = 0; i < N; i++) {
		if (!*names[i]) // shm_open("") is EINVAL, see below
			continue;
		fds[i] = shm_open(names[i], O_CREAT | O_EXCL | O_RDWR, 0600);
		if (fds[i] < 0) {
			printf("FAIL shm_open(\"%s\") create among distinct names: errno %d\n", names[i], errno);
			failures++;
		}
	}
	// Remove one at a time: the others must stay.
	for (int i = 0; i < N; i++) {
		if (!*names[i])
			continue;
		if (shm_unlink(names[i]) != 0) {
			printf("FAIL shm_unlink(\"%s\"): errno %d\n", names[i], errno);
			failures++;
		}
		for (int j = i + 1; j < N; j++) {
			if (!*names[j])
				continue;
			int fd = shm_open(names[j], O_RDWR, 0);
			if (fd < 0) {
				printf("FAIL removing \"%s\" removed \"%s\"\n", names[i], names[j]);
				failures++;
			} else {
				close(fd);
			}
		}
	}
	for (int i = 0; i < N; i++)
		if (fds[i] >= 0)
			close(fds[i]);

	// The same for semaphores.
	for (int i = 0; i < N; i++)
		sem_unlink(names[i]);
	sem_t* sems[N];
	for (int i = 0; i < N; i++) {
		sems[i] = sem_open(names[i], O_CREAT | O_EXCL, 0600, 0);
		if (sems[i] == SEM_FAILED) {
			printf("FAIL sem_open(\"%s\") create among distinct names: errno %d\n", names[i], errno);
			failures++;
		}
	}
	for (int i = 0; i < N; i++) {
		if (sem_unlink(names[i]) != 0) {
			printf("FAIL sem_unlink(\"%s\"): errno %d\n", names[i], errno);
			failures++;
		}
		for (int j = i + 1; j < N; j++) {
			sem_t* s = sem_open(names[j], 0);
			if (s == SEM_FAILED) {
				printf("FAIL removing semaphore \"%s\" removed \"%s\"\n", names[i], names[j]);
				failures++;
			} else {
				sem_close(s);
			}
		}
	}
	for (int i = 0; i < N; i++)
		if (sems[i] != SEM_FAILED)
			sem_close(sems[i]);
}

static void test_length_limits(void)
{
	shm_roundtrip(NAME31);
	sem_roundtrip(NAME30);
	CHECK_ERR(shm_open(NAME32, O_CREAT | O_RDWR, 0600), ENAMETOOLONG);
	CHECK_ERR(shm_open(NAME32, O_RDWR, 0), ENAMETOOLONG);
	errno = 0;
	CHECK(sem_open(NAME31, O_CREAT, 0600, 1) == SEM_FAILED && errno == ENAMETOOLONG);
	errno = 0;
	CHECK(sem_open(NAME31, 0) == SEM_FAILED && errno == ENAMETOOLONG);
	CHECK_ERR(sem_unlink(NAME31), ENAMETOOLONG);
	CHECK_ERR(shm_open("", O_CREAT | O_RDWR, 0600), EINVAL);
	CHECK_ERR(shm_unlink(""), EINVAL);
}

// QUIRK pinned here: shm_unlink() with a name over the limit reports EINVAL (shm_open reports ENAMETOOLONG).
static void test_quirk_shm_unlink_long_name_einval(void)
{
	CHECK_ERR(shm_unlink(NAME32), EINVAL);
	CHECK_ERR(shm_unlink("/0123456789012345678901234567890123456789"), EINVAL);
}

// Two processes meet on a name with an inner slash.
static void test_across_fork(void)
{
	const char* name = "/MTX/fork0001";
	shm_unlink(name);
	sem_unlink(name);
	int fd = shm_open(name, O_CREAT | O_EXCL | O_RDWR, 0600);
	CHECK(fd >= 0);
	if (fd < 0)
		return;
	CHECK(ftruncate(fd, 4096) == 0);
	volatile int* p = map_shm(fd);
	CHECK(p != NULL);
	sem_t* s = sem_open(name, O_CREAT | O_EXCL, 0600, 0);
	CHECK(s != SEM_FAILED);
	if (!p || s == SEM_FAILED)
		return;
	pid_t pid = fork();
	if (pid == 0) {
		int cfd = shm_open(name, O_RDWR, 0);
		sem_t* cs = sem_open(name, 0);
		volatile int* cp = cfd >= 0 ? map_shm(cfd) : NULL;
		if (!cp || cs == SEM_FAILED)
			_exit(2);
		*cp = 77;
		sem_post(cs);
		_exit(0);
	}
	CHECK(pid > 0);
	CHECK(sem_wait(s) == 0);
	CHECK(*p == 77);
	int status = 0;
	CHECK(waitpid(pid, &status, 0) == pid && WIFEXITED(status) && WEXITSTATUS(status) == 0);
	shm_unlink(name);
	sem_unlink(name);
	sem_close(s);
	close(fd);
}

int main(void)
{
	test_inner_slash_names();
	test_names_are_distinct();
	test_length_limits();
	test_quirk_shm_unlink_long_name_einval();
	test_across_fork();
	if (failures) {
		printf("%d failure(s)\n", failures);
		return 1;
	}
	printf("PASS\n");
	return 0;
}
