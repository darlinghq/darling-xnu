// A syscall() number in class 7 (0x7000000 | n) is a binary translator call, not a kernel system call. A dyld
// that sees the translated bit in the commpage makes them while mapping images, and treats any failure as
// "no ahead of time translation available". They must fail with ENOSYS without Darling reporting an
// "Unimplemented syscall" line on stderr, while a genuinely unknown number keeps being reported.
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define TRANSLATOR_CALL_1 0x7000001
#define TRANSLATOR_CALL_4 0x7000004
#define TRANSLATOR_CALL_LAST 0x7ffffff
#define UNKNOWN_BSD_NUMBER 4000 // beyond the end of the BSD syscall table

extern int syscall(int number, ...);

static void expect(int condition, const char* what)
{
	if (!condition) {
		printf("FAIL: %s (errno %d)\n", what, errno);
		exit(1);
	}
}

// Runs syscall(number) with stderr redirected to a scratch file; returns the number of bytes written to stderr.
static long stderr_bytes(int number, int* result, int* error)
{
	char path[] = "/tmp/translator-syscalls-XXXXXX";
	int scratch = mkstemp(path);
	expect(scratch >= 0, "scratch file");
	int saved = dup(2);
	expect(saved >= 0, "dup stderr");
	expect(dup2(scratch, 2) == 2, "redirect stderr");

	errno = 0;
	*result = syscall(number, 0, 0, 0, 0, 0, 0);
	*error = errno;

	expect(dup2(saved, 2) == 2, "restore stderr");
	close(saved);
	long bytes = lseek(scratch, 0, SEEK_END);
	close(scratch);
	unlink(path);
	return bytes;
}

int main(void)
{
	int numbers[] = { TRANSLATOR_CALL_1, TRANSLATOR_CALL_4, TRANSLATOR_CALL_LAST };
	for (unsigned i = 0; i < sizeof(numbers) / sizeof(numbers[0]); i++) {
		int result, error;
		long bytes = stderr_bytes(numbers[i], &result, &error);
		expect(result == -1 && error == ENOSYS, "a translator call fails with ENOSYS");
		expect(bytes == 0, "a translator call reports nothing on stderr");
	}

	// the report for a number beyond the BSD table is kept
	int result, error;
	long bytes = stderr_bytes(UNKNOWN_BSD_NUMBER, &result, &error);
	expect(result == -1 && error == ENOSYS, "an unknown BSD number fails with ENOSYS");
	expect(bytes > 0, "an unknown BSD number is still reported on stderr");

	printf("PASS: translator calls fail with ENOSYS silently\n");
	return 0;
}
