/* Compile and execute inside the selected Darling image, not on Linux. */
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define REQUIRE(condition) do { if (!(condition)) { \
	fprintf(stderr, "MADVISE_ADVICE_FAIL line=%d expression=%s errno=%d\n", \
		__LINE__, #condition, errno); \
	exit(1); } } while (0)

static int all_bytes(const unsigned char* memory, size_t size, unsigned char value)
{
	for (size_t i = 0; i < size; ++i)
		if (memory[i] != value)
			return 0;
	return 1;
}

int main(void)
{
	const long page_size = sysconf(_SC_PAGESIZE);
	REQUIRE(page_size > 0);
	const size_t page = (size_t)page_size;
	unsigned char* memory = mmap(NULL, 3 * page, PROT_READ | PROT_WRITE,
		MAP_PRIVATE | MAP_ANON, -1, 0);
	REQUIRE(memory != MAP_FAILED);
	memset(memory, 0x35, page);
	memset(memory + page, 0xa5, page);
	memset(memory + 2 * page, 0x71, page);

	/* MADV_ZERO is 11 in newer Darwin SDKs. Success is a promise that the
	 * selected private memory was zeroed, not merely that advice was ignored. */
	errno = 0;
	const int zero_rc = madvise(memory + page, page, 11);
	const int zero_errno = errno;
	printf("MADVISE_ZERO rc=%d errno=%d\n", zero_rc, zero_errno);
	if (zero_rc == 0)
		REQUIRE(all_bytes(memory + page, page, 0));
	else {
		REQUIRE(zero_rc == -1);
		REQUIRE(zero_errno == EINVAL || zero_errno == ENOTSUP);
		REQUIRE(all_bytes(memory + page, page, 0xa5));
	}
	REQUIRE(all_bytes(memory, page, 0x35));
	REQUIRE(all_bytes(memory + 2 * page, page, 0x71));

	const int invalid_advice[] = { INT_MAX, -1 };
	for (size_t i = 0; i < sizeof(invalid_advice) / sizeof(invalid_advice[0]); ++i) {
		memset(memory + page, 0xa5, page);
		errno = 0;
		REQUIRE(madvise(memory + page, page, invalid_advice[i]) == -1);
		REQUIRE(errno == EINVAL);
		REQUIRE(all_bytes(memory + page, page, 0xa5));
	}

	const int supported_advice[] = { MADV_NORMAL, MADV_RANDOM, MADV_SEQUENTIAL, MADV_WILLNEED };
	for (size_t i = 0; i < sizeof(supported_advice) / sizeof(supported_advice[0]); ++i) {
		REQUIRE(madvise(memory + page, page, supported_advice[i]) == 0);
		REQUIRE(all_bytes(memory + page, page, 0xa5));
	}
	REQUIRE(madvise(memory + page, page, MADV_DONTNEED) == 0);
	REQUIRE(all_bytes(memory, page, 0x35));
	REQUIRE(all_bytes(memory + 2 * page, page, 0x71));
	REQUIRE(munmap(memory, 3 * page) == 0);
	puts("MADVISE_ADVICE_PASS zero_contract=1 invalid=2 supported=5 neighbours=1");
	return 0;
}
