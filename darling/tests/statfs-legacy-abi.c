// Copyright (C) 2026 Lubos Dolezel
// The statfs family has two structure layouts: statfs, fstatfs and getfsstat (without 64-bit inodes) fill the
// legacy struct statfs (short and long fields, names of at most 90 bytes), while the $INODE64 and *64 entry points
// fill struct statfs64 (fixed-width fields, 1024-byte names). A 32-bit process gets the legacy layout from the
// plain symbols. Both layouts are declared here so the program does not depend on what an SDK maps statfs to,
// and the entry points are looked up by symbol name. It checks that the layouts hold the same information, that
// getfsstat takes its buffer size in bytes of the structure it fills (count with a NULL buffer, whole entries
// only), and that 32-bit counts that don't fit are all ones (the program also passes on macOS).
// The legacy layout is checked in both process widths: `long` is 4 bytes in a 32-bit process (272 bytes in all) and
// 8 in a 64-bit one (336 bytes, the same field order and name sizes; its counts never need clamping).
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct legacy_statfs {
	short f_otype;
	short f_oflags;
	long f_bsize;
	long f_iosize;
	long f_blocks;
	long f_bfree;
	long f_bavail;
	long f_files;
	long f_ffree;
	int32_t f_fsid[2];
	uint32_t f_owner;
	short f_reserved1;
	short f_type;
	long f_flags;
	long f_reserved2[2];
	char f_fstypename[15];
	char f_mntonname[90];
	char f_mntfromname[90];
	char f_reserved3;
	long f_reserved4[4];
};

struct wide_statfs {
	uint32_t f_bsize;
	int32_t f_iosize;
	uint64_t f_blocks;
	uint64_t f_bfree;
	uint64_t f_bavail;
	uint64_t f_files;
	uint64_t f_ffree;
	int32_t f_fsid[2];
	uint32_t f_owner;
	uint32_t f_type;
	uint32_t f_flags;
	uint32_t f_fssubtype;
	char f_fstypename[16];
	char f_mntonname[1024];
	char f_mntfromname[1024];
	uint32_t f_flags_ext;
	uint32_t f_reserved[7];
};

#define MNT_NOWAIT_ 2
#define MNT_WAIT_ 1

static int failures;
#define CHECK(c) do { if (!(c)) { printf("FAIL: %s (line %d, errno %d)\n", #c, __LINE__, errno); failures++; } } while (0)

typedef int (*statfs_fn)(const char*, void*);
typedef int (*fstatfs_fn)(int, void*);
typedef int (*getfsstat_fn)(void*, int, int);

static void* sym(const char* name)
{
	return dlsym(RTLD_DEFAULT, name);
}

static size_t untouched(const void* p, size_t n)
{
	size_t c = 0;
	for (size_t i = 0; i < n; i++)
		c += ((const unsigned char*) p)[i] == 0xAA;
	return c;
}

// a legacy count against the wide one: equal if it fits the width of long
static void check_count_clamp(const char* what, unsigned long long wide, long legacy)
{
	if (sizeof(long) == 8) {
		if ((unsigned long long) legacy != wide) { printf("FAIL: %s: %llu became %ld\n", what, wide, legacy); failures++; }
	} else if (wide <= 0xffffffffULL) {
		if ((unsigned long) legacy != (unsigned long) wide) { printf("FAIL: %s: %llu became %lu\n", what, wide, (unsigned long) legacy); failures++; }
	} else if (wide >= (1ULL << 62)) {
		// QUIRK pinned here: the largest counts (a filesystem without an inode limit) come out as all ones
		if (legacy != -1) { printf("FAIL: %s: %llu became %ld, not all ones\n", what, wide, legacy); failures++; }
	}
}

static void check_legacy(const struct legacy_statfs* l, const struct wide_statfs* w)
{
	CHECK(l->f_otype == 0);
	CHECK(l->f_oflags == 0);
	CHECK(l->f_reserved2[0] == 0 && l->f_reserved2[1] == 0);
	CHECK((unsigned long) l->f_bsize == w->f_bsize);
	CHECK(l->f_iosize == w->f_iosize);
	CHECK(l->f_owner == w->f_owner);
	CHECK(l->f_type == (short) w->f_type);
	CHECK((unsigned long) l->f_flags == w->f_flags);
	CHECK(l->f_fsid[0] == w->f_fsid[0] && l->f_fsid[1] == w->f_fsid[1]);
	check_count_clamp("f_blocks", w->f_blocks, l->f_blocks);
	check_count_clamp("f_files", w->f_files, l->f_files);
	CHECK(strncmp(l->f_fstypename, w->f_fstypename, sizeof l->f_fstypename - 1) == 0);
	CHECK(strncmp(l->f_mntonname, w->f_mntonname, sizeof l->f_mntonname - 1) == 0);
	CHECK(strncmp(l->f_mntfromname, w->f_mntfromname, sizeof l->f_mntfromname - 1) == 0);
	CHECK(l->f_fstypename[sizeof l->f_fstypename - 1] == 0);
	CHECK(l->f_mntonname[sizeof l->f_mntonname - 1] == 0);
	CHECK(l->f_mntfromname[sizeof l->f_mntfromname - 1] == 0);
}

static void check_wide_names(const struct wide_statfs* w)
{
	CHECK(strcmp(w->f_mntonname, "/") == 0);
	CHECK(w->f_fstypename[0] != 0);
	CHECK(w->f_mntfromname[0] != 0);
	CHECK(w->f_bsize != 0);
	CHECK(w->f_blocks != 0);
}

int main(void)
{
	statfs_fn statfs_i64 = sym("statfs$INODE64"), statfs_64 = sym("statfs64");
	fstatfs_fn fstatfs_i64 = sym("fstatfs$INODE64"), fstatfs_64 = sym("fstatfs64");
	getfsstat_fn getfsstat_i64 = sym("getfsstat$INODE64"), getfsstat_64 = sym("getfsstat64");
	struct wide_statfs w, w2;
	int fd = open("/", O_RDONLY);
	CHECK(fd >= 0);
	CHECK(statfs_i64 != NULL && fstatfs_i64 != NULL && getfsstat_i64 != NULL);

	// statfs64 layout: every entry point agrees, and no byte is left as the caller had it
	memset(&w, 0xAA, sizeof w);
	CHECK(statfs_i64("/", &w) == 0);
	check_wide_names(&w);
	CHECK(untouched(&w, sizeof w) < 8);
	CHECK(w.f_owner == 0);
	memset(&w2, 0xAA, sizeof w2);
	CHECK(fstatfs_i64(fd, &w2) == 0);
	check_wide_names(&w2);
	CHECK(w2.f_blocks == w.f_blocks && w2.f_type == w.f_type && w2.f_bsize == w.f_bsize);
	CHECK(strcmp(w2.f_fstypename, w.f_fstypename) == 0 && strcmp(w2.f_mntfromname, w.f_mntfromname) == 0);
	if (statfs_64) {
		memset(&w2, 0xAA, sizeof w2);
		CHECK(statfs_64("/", &w2) == 0);
		CHECK(w2.f_blocks == w.f_blocks && w2.f_type == w.f_type && strcmp(w2.f_mntonname, "/") == 0);
	}
	if (fstatfs_64) {
		memset(&w2, 0xAA, sizeof w2);
		CHECK(fstatfs_64(fd, &w2) == 0);
		CHECK(w2.f_blocks == w.f_blocks && strcmp(w2.f_mntonname, "/") == 0);
	}
	errno = 0;
	CHECK(statfs_i64("/nonexistent-statfs-abi-path", &w2) == -1 && errno == ENOENT);
	errno = 0;
	CHECK(fstatfs_i64(-1, &w2) == -1 && errno == EBADF);

	// getfsstat with statfs64 entries: bufsize is in bytes of the structure
	int n = getfsstat_i64(NULL, 0, MNT_NOWAIT_);
	CHECK(n >= 1);
	CHECK(getfsstat_i64(NULL, 0, MNT_WAIT_) == n);
	struct wide_statfs* wl = malloc((n + 3) * sizeof *wl);
	memset(wl, 0xAA, (n + 3) * sizeof *wl);
	CHECK(getfsstat_i64(wl, (n + 3) * sizeof *wl, MNT_NOWAIT_) == n);
	CHECK(strcmp(wl[0].f_mntonname, "/") == 0);
	CHECK(untouched(&wl[n], sizeof *wl) == sizeof *wl);
	CHECK(getfsstat_i64(wl, 0, MNT_NOWAIT_) == 0);
	CHECK(getfsstat_i64(wl, sizeof *wl - 1, MNT_NOWAIT_) == 0);
	CHECK(getfsstat_i64(wl, sizeof *wl, MNT_NOWAIT_) == 1);
	CHECK(getfsstat_i64(wl, (n + 3) * sizeof *wl, MNT_WAIT_) == n);
	if (getfsstat_64) {
		CHECK(getfsstat_64(NULL, 0, MNT_NOWAIT_) == n);
		CHECK(getfsstat_64(wl, sizeof *wl, MNT_NOWAIT_) == 1);
	}
	if (n >= 2)
		CHECK(getfsstat_i64(wl, 2 * sizeof *wl + 7, MNT_NOWAIT_) == 2);

	// the plain symbols: the legacy layout
	statfs_fn statfs_legacy = sym("statfs");
	fstatfs_fn fstatfs_legacy = sym("fstatfs");
	getfsstat_fn getfsstat_legacy = sym("getfsstat");
	struct legacy_statfs l;
	CHECK(sizeof l == (sizeof(long) == 4 ? 272 : 336));
	CHECK(offsetof(struct legacy_statfs, f_mntonname) == (sizeof(long) == 4 ? 75 : 119));
	CHECK(statfs_legacy != NULL && fstatfs_legacy != NULL && getfsstat_legacy != NULL);

	memset(&l, 0xAA, sizeof l);
	CHECK(statfs_legacy("/", &l) == 0);
	CHECK(strcmp(l.f_mntonname, "/") == 0);
	check_legacy(&l, &w);
	// the counts are the real ones (a volume whose block count is below 2^32 reports it unchanged)
	CHECK(l.f_blocks != 0);
	CHECK(l.f_bsize != 0);

	memset(&l, 0xAA, sizeof l);
	CHECK(fstatfs_legacy(fd, &l) == 0);
	CHECK(strcmp(l.f_mntonname, "/") == 0);
	check_legacy(&l, &w);

	errno = 0;
	CHECK(statfs_legacy("/nonexistent-statfs-abi-path", &l) == -1 && errno == ENOENT);
	errno = 0;
	CHECK(fstatfs_legacy(-1, &l) == -1 && errno == EBADF);

	// getfsstat: same count and order as the statfs64 entries, entries are sizeof(legacy) apart
	CHECK(getfsstat_legacy(NULL, 0, MNT_NOWAIT_) == n);
	CHECK(getfsstat_legacy(NULL, 0, MNT_WAIT_) == n);
	struct legacy_statfs* ll = malloc((n + 3) * sizeof *ll);
	memset(ll, 0xAA, (n + 3) * sizeof *ll);
	CHECK(getfsstat_legacy(ll, (n + 3) * sizeof *ll, MNT_NOWAIT_) == n);
	CHECK(untouched(&ll[n], sizeof *ll) == sizeof *ll);
	for (int i = 0; i < n; i++)
		check_legacy(&ll[i], &wl[i]);
	CHECK(getfsstat_legacy(ll, (n + 3) * sizeof *ll, MNT_WAIT_) == n);
	CHECK(getfsstat_legacy(ll, 0, MNT_NOWAIT_) == 0);
	CHECK(getfsstat_legacy(ll, sizeof *ll - 1, MNT_NOWAIT_) == 0);
	CHECK(getfsstat_legacy(ll, sizeof *ll, MNT_NOWAIT_) == 1);
	if (n >= 2) {
		memset(ll, 0xAA, 3 * sizeof *ll);
		CHECK(getfsstat_legacy(ll, 2 * sizeof *ll + 7, MNT_NOWAIT_) == 2);
		CHECK(untouched(&ll[2], sizeof *ll) == sizeof *ll);
	}
	free(ll);
	free(wl);
	close(fd);
	if (failures == 0)
		printf("PASS\n");
	return failures != 0;
}
