// Copyright (C) 2026 Lubos Dolezel
// madvise() widens its range to whole pages instead of rejecting an
// unaligned address, treats length 0 as a no-op, and reports EINVAL for the
// reuse hint on a range that is not entirely mapped. Expected values are
// black-box observations of macOS 26.7.3 x86_64.
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>

static int failures;
#define CHECK(c) do { if (!(c)) { printf("FAIL: %s (line %d, errno %d)\n", #c, __LINE__, errno); failures++; } } while (0)

int main(void)
{
	size_t pg = (size_t) getpagesize();
	char *p = mmap(NULL, 3 * pg, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
	CHECK(p != MAP_FAILED);
	CHECK(munmap(p + 2 * pg, pg) == 0);
	memset(p, 7, 2 * pg);

	// unaligned start succeeds for each hint, and the data stays intact
	CHECK(madvise(p + 1, pg, MADV_WILLNEED) == 0);
	CHECK(madvise(p + 100, 10, MADV_FREE_REUSABLE) == 0);
	CHECK(madvise(p + 100, 10, MADV_FREE_REUSE) == 0);
	CHECK(madvise(p + 1, pg, MADV_FREE_REUSE) == 0);
	CHECK(p[0] == 7 && p[pg] == 7);

	// test_quirk_madvise_reuse_unmapped_einval
	errno = 0;
	CHECK(madvise(p + 2 * pg, pg, MADV_FREE_REUSE) == -1 && errno == EINVAL);
	errno = 0;
	CHECK(madvise(p + pg + 1, pg, MADV_FREE_REUSE) == -1 && errno == EINVAL);

	// zero length is a no-op even when the address is unaligned
	CHECK(madvise(p + 1, 0, MADV_FREE_REUSE) == 0);

	if (failures) return 1;
	printf("PASS\n");
	return 0;
}
