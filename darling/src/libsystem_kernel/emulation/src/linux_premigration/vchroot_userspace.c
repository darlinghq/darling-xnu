
#include <darling/emulation/linux_premigration/vchroot_expand.h>
#include "name_equivalence.h"
#ifdef TEST
#	include <unistd.h>
#	include <sys/stat.h>
#	include <sys/syscall.h>
#	include <stdio.h>
#	include <stdbool.h>
#	include <stdlib.h>
#	include <string.h>
#	include <errno.h>
#	include <dirent.h>
#	include <fcntl.h>

#	define LINUX_SYSCALL(...) ({ int __rv = syscall(__VA_ARGS__); if (__rv == -1) __rv = -errno; __rv; })
#	define LINUX_ENOENT ENOENT
#	define LINUX_ELOOP ELOOP
#	define LINUX_ENAMETOOLONG ENAMETOOLONG

#	define LINUX_S_IFMT S_IFMT
#	define LINUX_S_IFLNK S_IFLNK

#	define LINUX_O_RDONLY O_RDONLY

#	define __simple_sprintf sprintf
#	define linux_dirent64 dirent
typedef struct stat stat_t;
#else
#	include <stdbool.h>
#   include <stddef.h>

#	include <darling/emulation/xnu_syscall/bsd/impl/fcntl/open.h>
#	include <darling/emulation/common/base.h>
#	include <darling/emulation/other/mach/lkm.h>
#	include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#	include <darling/emulation/common/simple.h>
#	include <darling/emulation/conversion/dirent/getdirentries.h>
#	include <darling/emulation/conversion/errno.h>
#	include <darling/emulation/conversion/fcntl/open.h>
#	include <darling/emulation/conversion/duct_errno.h>
#	include <darling/emulation/conversion/stat/common.h>
#	include <darling/emulation/xnu_syscall/bsd/impl/dirent/getdirentries.h>
#	include <darling/emulation/conversion/common_at.h>

#include <darlingserver/rpc.h>

typedef struct linux_stat stat_t;

struct _xlocale;
typedef struct _xlocale* locale_t;

#define LC_C_LOCALE ((locale_t)NULL)

extern __SIZE_TYPE__ strlen(const char* s);
extern char* strcpy(char* dest, const char* src);
extern int strcmp(const char* str1, const char* str2);
extern int strncmp(const char* str1, const char* str2, __SIZE_TYPE__ n);
extern int strncasecmp_l(const char* str1, const char* str2, __SIZE_TYPE__ n, locale_t locale);
extern char* strcat(char* dest, const char* src);
extern char *strchr(const char *s, int c);
extern void *memcpy(void *dest, const void *src, __SIZE_TYPE__ n);
extern void* memmove(void* dest, const void* src, __SIZE_TYPE__ n);
extern int memcmp(const void* dest, const void* src, __SIZE_TYPE__ n);

#define LINUX_S_IFMT 00170000
#define LINUX_S_IFLNK 0120000

#endif

#include <stdint.h>

static bool next_component(const char* from, const char** end);

static char prefix_path[4096] = {0}; // MUST NOT end with '/'
static int prefix_path_len = -1;

static const char EXIT_PATH[] = "/Volumes/SystemRoot";

#ifndef TEST
#ifdef VARIANT_DYLD
#define icase_enabled false
#else
#define icase_enabled true
#endif

#else
static bool icase_enabled = true;
#endif

#if defined(TEST) || !defined(VARIANT_DYLD)
#ifndef TEST
#include <darling/emulation/linux_premigration/ext/sys/linux_time.h>
#else
#include <time.h>
#endif

#define ICASE_CACHE_SETS 256
#define ICASE_CACHE_WAYS 4
#define ICASE_NAME_MAX 256

struct icase_cache_entry {
	uint64_t dev;
	uint64_t ino;
	uint64_t mtime_sec;
	uint64_t mtime_nsec;
	uint64_t ctime_sec;
	uint64_t ctime_nsec;
	int64_t size;
	uint64_t nlink;
	uint32_t hash;
	uint32_t last_used;
	uint16_t stored_len;
	bool valid;
	bool found;
	char key_name[ICASE_NAME_MAX];
	char stored_name[ICASE_NAME_MAX];
};

static struct icase_cache_entry icase_cache[ICASE_CACHE_SETS][ICASE_CACHE_WAYS];
static volatile int icase_cache_lock = 0;
static uint32_t icase_cache_clock = 0;

static inline bool icase_trylock_bounded(void)
{
	for (int iter = 0; iter < 1024; iter++) {
		if (__atomic_exchange_n(&icase_cache_lock, 1, __ATOMIC_ACQUIRE) == 0)
			return true;
#if defined(__aarch64__)
		__asm__ volatile("yield" ::: "memory");
#elif defined(__x86_64__) || defined(__i386__)
		__asm__ volatile("pause" ::: "memory");
#endif
	}
	return false;
}

static inline void icase_unlock(void)
{
	__atomic_store_n(&icase_cache_lock, 0, __ATOMIC_RELEASE);
}

void vchroot_icase_postfork_child(void)
{
	for (int s = 0; s < ICASE_CACHE_SETS; s++) {
		for (int w = 0; w < ICASE_CACHE_WAYS; w++)
			icase_cache[s][w].valid = false;
	}
	icase_cache_clock = 0;
	__atomic_store_n(&icase_cache_lock, 0, __ATOMIC_RELEASE);
}

static inline uint32_t icase_hash_key(uint64_t dev, uint64_t ino, const char* name)
{
	uint64_t h = 14695981039346656037ULL;
	h ^= dev;
	h *= 1099511628211ULL;
	h ^= ino;
	h *= 1099511628211ULL;
	for (const unsigned char* p = (const unsigned char*)name; *p; p++) {
		unsigned char c = *p;
		if (c >= 'A' && c <= 'Z')
			c += 'a' - 'A';
		h ^= c;
		h *= 1099511628211ULL;
	}
	return (uint32_t)(h ^ (h >> 32));
}

static inline void stat_get_meta(const stat_t* st, uint64_t* m_sec, uint64_t* m_nsec,
                                 uint64_t* c_sec, uint64_t* c_nsec,
                                 int64_t* size, uint64_t* nlink)
{
#ifdef TEST
	*m_sec = (uint64_t)st->st_mtim.tv_sec;
	*m_nsec = (uint64_t)st->st_mtim.tv_nsec;
	*c_sec = (uint64_t)st->st_ctim.tv_sec;
	*c_nsec = (uint64_t)st->st_ctim.tv_nsec;
#else
	*m_sec = (uint64_t)st->st_mtime;
	*m_nsec = (uint64_t)st->st_mtime_nsec;
	*c_sec = (uint64_t)st->st_ctime;
	*c_nsec = (uint64_t)st->st_ctime_nsec;
#endif
	*size = (int64_t)st->st_size;
	*nlink = (uint64_t)st->st_nlink;
}

static inline bool icase_dir_settled(uint64_t mtime_sec)
{
	struct timespec now;
#ifdef TEST
	if (clock_gettime(CLOCK_REALTIME, &now) != 0)
		return false;
#else
	if (__darling_vdso_clock_gettime(0 /* CLOCK_REALTIME */, &now) != 0) {
		if (LINUX_SYSCALL(__NR_clock_gettime, 0 /* CLOCK_REALTIME */, &now) < 0)
			return false;
	}
#endif
	return now.tv_sec >= 0 && (uint64_t)now.tv_sec >= mtime_sec &&
	       ((uint64_t)now.tv_sec - mtime_sec) >= 2;
}

static bool icase_cache_lookup(const stat_t* dir_st, const char* comp,
                               bool* out_found, char* out_name, uint16_t* out_len)
{
	uint64_t m_sec, m_nsec, c_sec, c_nsec, nlink;
	int64_t size;
	stat_get_meta(dir_st, &m_sec, &m_nsec, &c_sec, &c_nsec, &size, &nlink);
	uint64_t dev = (uint64_t)dir_st->st_dev;
	uint64_t ino = (uint64_t)dir_st->st_ino;
	uint32_t hash = icase_hash_key(dev, ino, comp);
	uint32_t set = hash & (ICASE_CACHE_SETS - 1);
	bool hit = false;

	if (!icase_trylock_bounded())
		return false;
	for (int w = 0; w < ICASE_CACHE_WAYS; w++) {
		struct icase_cache_entry* e = &icase_cache[set][w];
		if (!e->valid || e->hash != hash || e->dev != dev || e->ino != ino)
			continue;
		if (e->mtime_sec != m_sec || e->mtime_nsec != m_nsec ||
		    e->ctime_sec != c_sec || e->ctime_nsec != c_nsec ||
		    e->size != size || e->nlink != nlink) {
			e->valid = false;
			continue;
		}
		if (darling_names_equivalent(e->key_name, comp)) {
			e->last_used = ++icase_cache_clock;
			*out_found = e->found;
			if (e->found) {
				*out_len = e->stored_len;
				memcpy(out_name, e->stored_name, e->stored_len + 1);
			}
			hit = true;
			break;
		}
	}
	icase_unlock();
	return hit;
}

static void icase_cache_insert(const stat_t* dir_st, const char* comp, size_t comp_len,
                               bool found, const char* stored_name, size_t stored_len)
{
	if (comp_len == 0 || comp_len >= ICASE_NAME_MAX)
		return;
	if (found && stored_len >= ICASE_NAME_MAX)
		return;

	uint64_t m_sec, m_nsec, c_sec, c_nsec, nlink;
	int64_t size;
	stat_get_meta(dir_st, &m_sec, &m_nsec, &c_sec, &c_nsec, &size, &nlink);

	/* Only remember answers about a directory that has not changed for 2
	 * seconds. The modification time is the only thing that tells a later
	 * lookup that an entry was added, removed or renamed, and it advances in
	 * coarse steps (a few milliseconds), so a change made right after an answer
	 * was cached can leave every recorded attribute as it was. That would keep
	 * a name missing that has just been created (a negative answer), or keep a
	 * name that has just been replaced by another spelling of it (a positive
	 * one). Once the directory is older than the clock's step, any later change
	 * gives it a different time. */
	if (!icase_dir_settled(m_sec))
		return;

	uint64_t dev = (uint64_t)dir_st->st_dev;
	uint64_t ino = (uint64_t)dir_st->st_ino;
	uint32_t hash = icase_hash_key(dev, ino, comp);
	uint32_t set = hash & (ICASE_CACHE_SETS - 1);

	if (!icase_trylock_bounded())
		return;
	int victim = 0;
	uint32_t oldest = UINT32_MAX;
	for (int w = 0; w < ICASE_CACHE_WAYS; w++) {
		struct icase_cache_entry* e = &icase_cache[set][w];
		if (!e->valid) {
			victim = w;
			break;
		}
		if (e->hash == hash && e->dev == dev && e->ino == ino &&
		    darling_names_equivalent(e->key_name, comp)) {
			victim = w;
			break;
		}
		if (e->last_used < oldest) {
			oldest = e->last_used;
			victim = w;
		}
	}

	struct icase_cache_entry* dst = &icase_cache[set][victim];
	dst->dev = dev;
	dst->ino = ino;
	dst->mtime_sec = m_sec;
	dst->mtime_nsec = m_nsec;
	dst->ctime_sec = c_sec;
	dst->ctime_nsec = c_nsec;
	dst->size = size;
	dst->nlink = nlink;
	dst->hash = hash;
	dst->last_used = ++icase_cache_clock;
	dst->valid = true;
	dst->found = found;
	memcpy(dst->key_name, comp, comp_len + 1);
	if (found) {
		dst->stored_len = (uint16_t)stored_len;
		memcpy(dst->stored_name, stored_name, stored_len + 1);
	} else {
		dst->stored_len = 0;
	}
	icase_unlock();
}

__attribute__((noinline))
static bool icase_scan_dir(const char* parent_dir, const stat_t* parent_st, int parent_status,
                           const char* comp, size_t comp_len,
                           char* out_name, size_t out_cap, int* out_len)
{
#if defined(__NR_open)
	int dfd = LINUX_SYSCALL(__NR_open, parent_dir, LINUX_O_RDONLY);
#else
	int dfd = LINUX_SYSCALL(__NR_openat, LINUX_AT_FDCWD, parent_dir, LINUX_O_RDONLY);
#endif
	if (dfd < 0)
		return false;

	char dirents[8192]; // 8KB buffer space for struct linux_dirent64 entries
	int dlen;
	bool matched = false;

	while ((dlen = LINUX_SYSCALL(__NR_getdents64, dfd, dirents, sizeof(dirents))) > 0)
	{
		int pos = 0;
		while (pos < dlen)
		{
			struct linux_dirent64* de = (struct linux_dirent64*) &dirents[pos];
			if (darling_names_equivalent(de->d_name, comp))
			{
				const int stored_len = strlen(de->d_name);
				if ((size_t)stored_len + 2 <= out_cap) {
					if (parent_status == 0)
						icase_cache_insert(parent_st, comp, comp_len, true, de->d_name, (size_t)stored_len);
					memcpy(out_name, de->d_name, (size_t)stored_len + 1);
					*out_len = stored_len;
					matched = true;
				}
				goto out;
			}
			pos += de->d_reclen;
		}
	}
	if (dlen == 0 && parent_status == 0)
		icase_cache_insert(parent_st, comp, comp_len, false, "", 0);

out:
	LINUX_SYSCALL(__NR_close, dfd);
	return matched;
}
#endif

static const int MAX_SYMLINK_DEPTH = 10;

struct context
{
	const char* current_root;
	int current_root_len;

	char current_path[4096];
	int current_path_len;

	int symlink_depth;
	bool unknown_component;
	bool follow;
};

static int vchroot_run(const char* path, struct context* ctxt);

#ifndef TEST
VISIBLE
#endif
int __darling_vchroot(int dfd)
{
	int rv;

#ifndef TEST
	rv = errno_linux_to_bsd(dserver_rpc_vchroot(dfd));
	if (rv != 0)
		return rv;
#endif

	char buf[50];

	__simple_sprintf(buf, "/proc/self/fd/%d", dfd);

	#if defined(__NR_readlink)
		rv = LINUX_SYSCALL(__NR_readlink, buf, prefix_path, sizeof(prefix_path) - 1);
	#else
		rv = LINUX_SYSCALL(__NR_readlinkat, LINUX_AT_FDCWD, buf, prefix_path, sizeof(prefix_path) - 1);
	#endif
	prefix_path[rv] = '\0';
	prefix_path_len = rv;

	return 0;
}

#ifndef TEST
VISIBLE
int __darling_vchroot_expand(const char* path, char* out)
{
	struct vchroot_expand_args a = {
		.dfd = -100,
		.flags = 0
	};

	strcpy(a.path, path);

	int rv = vchroot_expand(&a);

	strcpy(out, a.path);
	return rv;
}
#endif

static void init_vchroot_path(void)
{
	uint64_t tmp_len;
	int code = dserver_rpc_vchroot_path(prefix_path, sizeof(prefix_path), &tmp_len);
	if (code < 0) {
		__simple_kprintf("Failed to get vchroot path: %d\n", code);
		__simple_abort();
	}

	prefix_path_len = (int)tmp_len;
}

int vchroot_expand(struct vchroot_expand_args* args)
{
	struct context ctxt;

#ifndef TEST
	if (prefix_path_len == -1)
		init_vchroot_path();
#endif

	ctxt.unknown_component = false;
	ctxt.symlink_depth = 0;
	ctxt.current_root = prefix_path;
	ctxt.current_root_len = prefix_path_len;
	ctxt.follow = !!(args->flags & VCHROOT_FOLLOW);

	const char* input_path = args->path;

	if (*input_path == '\0')
		return -LINUX_ENOENT;

	if (*input_path != '/')
	{
		char buf[50];

		if (args->dfd != -100)
			__simple_sprintf(buf, "/proc/self/fd/%d", args->dfd);
		else
			strcpy(buf, "/proc/self/cwd");

		#if defined(__NR_readlink)
			int rv = LINUX_SYSCALL(__NR_readlink, buf, ctxt.current_path, sizeof(ctxt.current_path) - 1);
		#else
			int rv = LINUX_SYSCALL(__NR_readlinkat, LINUX_AT_FDCWD, buf, ctxt.current_path, sizeof(ctxt.current_path) - 1);
		#endif

		if (rv < 0)
			return rv;
		
		// If we got "/", then turn it into "", because our algorithm will add a trailing slash later
		if (rv == 1 && ctxt.current_path[0] == '/')
		{
			ctxt.current_path[0] = '\0';
			ctxt.current_path_len = 0;
		}
		else
		{
			ctxt.current_path_len = rv;
			ctxt.current_path[rv] = '\0';
		}

		// Are we outside the vchrooted area already?
		if (ctxt.current_path_len < ctxt.current_root_len || strncmp(ctxt.current_path, ctxt.current_root, ctxt.current_root_len) != 0)
		{
			ctxt.current_root = "";
			ctxt.current_root_len = 0;
		}
	}

	int rv = vchroot_run(input_path, &ctxt);
	if (ctxt.current_path_len > 0)
		strcpy(args->path, ctxt.current_path);
	else
		strcpy(args->path, "/");

	return rv;
}

static int vchroot_run(const char* input_path, struct context* ctxt)
{
	if (ctxt->symlink_depth > MAX_SYMLINK_DEPTH)
		return -LINUX_ELOOP;

	if (*input_path == '/')
	{
		strcpy(ctxt->current_path, ctxt->current_root);
		ctxt->current_path_len = strlen(ctxt->current_path);
		input_path++;
	}

	const char* end;
	while (next_component(input_path, &end))
	{
		const unsigned int len = end - input_path;

		if ((len == 1 && input_path[0] == '.') || len == 0) // . or nothing (//)
		{
			// nothing to do
		}
		else
		{
			// append /
			ctxt->current_path[ctxt->current_path_len++] = '/';
			ctxt->current_path[ctxt->current_path_len] = '\0';

			if (len == 2 && input_path[0] == '.' && input_path[1] == '.') // ..
			{
				// Returning back into the vchrooted area
				if (ctxt->current_path_len <= 1)
				{
					strcpy(ctxt->current_path, prefix_path);
					strcat(ctxt->current_path, EXIT_PATH);
					ctxt->current_path_len = prefix_path_len + sizeof(EXIT_PATH);

					ctxt->current_path[ctxt->current_path_len-1] = '/';
					ctxt->current_path[ctxt->current_path_len] = '\0';

					ctxt->current_root = prefix_path;
					ctxt->current_root_len = prefix_path_len;
				}

				// current_path always ends with a slash
				// p points before the last /
				char* p = ctxt->current_path + ctxt->current_path_len - 2;

				while (true)
				{
					if (*p == '/')
					{
						*p = '\0';
						ctxt->current_path_len = p - ctxt->current_path;

						// We cannot exit current_root via ..
						if (ctxt->current_path_len < ctxt->current_root_len)
						{
							// Post-pop safety clamp: if popping a path component
							// left the path shallower than current_root, clamp back to current_root
							// rather than failing with -LINUX_ENOENT (which broke cd .. from root).
							strcpy(ctxt->current_path, ctxt->current_root);
							ctxt->current_path_len = ctxt->current_root_len;
						}
						break;
					}
					p--;
				}
			}
			else
			{
				if (len + ctxt->current_path_len + 2 > sizeof(ctxt->current_path))
					return -LINUX_ENOENT;
				
				// Append the component
				const int prevlen = ctxt->current_path_len;

				memcpy(ctxt->current_path + ctxt->current_path_len, input_path, len);
				ctxt->current_path_len += len;
				ctxt->current_path[ctxt->current_path_len] = '\0';

				// Only analyze the last path component if we haven't had an unresolvent ENOENT before
				if (!ctxt->unknown_component)
				{
					stat_t st;
					// struct linux_stat has the stat64 layout on i386, so
					// prefer lstat64 there; the older lstat fills a smaller
					// struct and st_mode would be read from the wrong offset.
					#if defined(__NR_lstat64)
						int status = LINUX_SYSCALL(__NR_lstat64, ctxt->current_path, &st);
					#elif defined(__NR_lstat)
						int status = LINUX_SYSCALL(__NR_lstat, ctxt->current_path, &st);
					#else
						#if __NR_newfstatat
							int status = LINUX_SYSCALL(__NR_newfstatat, LINUX_AT_FDCWD, ctxt->current_path, &st, LINUX_AT_SYMLINK_NOFOLLOW);
						#else
							int status = LINUX_SYSCALL(__NR_fstatat64, LINUX_AT_FDCWD, ctxt->current_path, &st, LINUX_AT_SYMLINK_NOFOLLOW);
						#endif
					#endif

					if (icase_enabled && status == -LINUX_ENOENT)
					{
						// Look for a name that is the same to a case- and normalization-insensitive file system, as
						// APFS is (see name_equivalence.c)
#if defined(TEST) || !defined(VARIANT_DYLD)
						// Examine the directory above
						ctxt->current_path[prevlen-1] = '\0';
						const char* parent_dir = (prevlen > 1) ? ctxt->current_path : "/";
						const char* comp = ctxt->current_path + prevlen;
						const size_t comp_len = (size_t)(ctxt->current_path_len - prevlen);

						stat_t parent_st;
						#if defined(__NR_lstat64)
							int parent_status = LINUX_SYSCALL(__NR_lstat64, parent_dir, &parent_st);
						#elif defined(__NR_lstat)
							int parent_status = LINUX_SYSCALL(__NR_lstat, parent_dir, &parent_st);
						#else
							#if __NR_newfstatat
								int parent_status = LINUX_SYSCALL(__NR_newfstatat, LINUX_AT_FDCWD, parent_dir, &parent_st, LINUX_AT_SYMLINK_NOFOLLOW);
							#else
								int parent_status = LINUX_SYSCALL(__NR_fstatat64, LINUX_AT_FDCWD, parent_dir, &parent_st, LINUX_AT_SYMLINK_NOFOLLOW);
							#endif
						#endif

						if (parent_status == 0) {
							bool cached_found = false;
							char cached_name[ICASE_NAME_MAX];
							uint16_t cached_len = 0;
							if (icase_cache_lookup(&parent_st, comp, &cached_found, cached_name, &cached_len)) {
								if (cached_found) {
									if (prevlen + cached_len + 2 <= sizeof(ctxt->current_path)) {
										memcpy(ctxt->current_path + prevlen, cached_name, cached_len + 1);
										ctxt->current_path_len = prevlen + cached_len;
									} else {
										ctxt->unknown_component = true;
									}
								} else {
									ctxt->unknown_component = true;
								}
								ctxt->current_path[prevlen-1] = '/';
								goto after_icase;
							}
						}

						int matched_len = 0;
						if (icase_scan_dir(parent_dir, &parent_st, parent_status,
						                   comp, comp_len,
						                   ctxt->current_path + prevlen,
						                   sizeof(ctxt->current_path) - prevlen,
						                   &matched_len)) {
							ctxt->current_path_len = prevlen + matched_len;
						} else {
							ctxt->unknown_component = true;
						}
						// Restore the / we removed a few lines above
						ctxt->current_path[prevlen-1] = '/';
after_icase:;
#endif
					}
					else if (status == 0 && (st.st_mode & LINUX_S_IFMT) == LINUX_S_IFLNK)
					{
						// Follow symlink if follow is true or if we haven't reached the end of the input path yet
						if (ctxt->follow || *end)
						{
							char link[512];
							int rv;

							#if defined(__NR_readlink)
								rv = LINUX_SYSCALL(__NR_readlink, ctxt->current_path, link, sizeof(link) - 1);
							#else
								rv = LINUX_SYSCALL(__NR_readlinkat, LINUX_AT_FDCWD, ctxt->current_path, link, sizeof(link) - 1);
							#endif

							if (rv < 0)
								return rv;

							link[rv] = '\0';

							// https://github.com/darlinghq/darling/issues/916
							// Special procfs hack due to the presence of broken symlinks in /proc/xxx/fd/ used to
							// point to sockets, pipes or anonymous inodes.
							if (strncmp(ctxt->current_path, "/proc", 5) == 0)
							{
								if (strchr(link, ':') != 0)
								{
									goto ignore_symlink;
								}
							}

							// Remove the last component (because it will be substituted with symlink contents)
							if (link[0] != '/') // Only bother to do that if we know that the symlink is not absolute
							{
								ctxt->current_path_len = prevlen - 1; // kill the last slash as well
								ctxt->current_path[ctxt->current_path_len] = '\0';
							}

							// Symbolic link resolution
							const bool orig_follow = ctxt->follow;

							ctxt->follow = true;
							ctxt->symlink_depth++;

							// resolve absolute symlinks within the prefix
							//
							// however, DON'T resolve `/proc` symlinks within the prefix.
							// anything under `/proc` is from Linux's `/proc` since we bind-mount it, so we can special-case them because we want those to be Linux-root relative.
							// see https://github.com/darlinghq/darling/issues/1052 for the issue this works around.
							//
							// symlinks found after leaving the prefix through /Volumes/SystemRoot
							// (current_root is empty) are host symlinks, so they stay relative
							// to the host root (e.g. /etc/resolv.conf -> /run/...).
							if (link[0] == '/' && ctxt->current_root_len > 0 && !(strncmp(link, "/proc/", 6) == 0 || (rv == 5 && strncmp(link, "/proc", 5) == 0))) {
								ctxt->current_root = prefix_path;
								ctxt->current_root_len = prefix_path_len;
							}

							rv = vchroot_run(link, ctxt);

							ctxt->symlink_depth--;
							ctxt->follow = orig_follow;

							if (rv != 0)
								return rv;
						}
					}
				}

ignore_symlink:

				if (ctxt->current_root_len > 0 && ctxt->current_path_len - ctxt->current_root_len == sizeof(EXIT_PATH)-1)
				{
					if (strncasecmp_l(ctxt->current_path + ctxt->current_root_len, EXIT_PATH, sizeof(EXIT_PATH) - 1, LC_C_LOCALE) == 0)
					{
						// Switch to the real system root
						ctxt->current_path[0] = '\0';
						ctxt->current_path_len = 0;
						ctxt->current_root = "";
						ctxt->current_root_len = 0;
					}
				}
			}
		}

		// Move on to the next component
		input_path = end;
		while (*input_path == '/')
			input_path++;
	}

	return 0;
}

static bool next_component(const char* from, const char** end)
{
	const char* pos = from;
	while (*pos != '/' && *pos)
		pos++;
	*end = pos;
	return *from;
}

int vchroot_fdpath(struct vchroot_fdpath_args* args)
{
#ifndef TEST
	if (prefix_path_len == -1)
		init_vchroot_path();
#endif

	char buf[50];
	char link[4096];

	__simple_sprintf(buf, "/proc/self/fd/%d", args->fd);

	#if defined(__NR_readlink)
		int rv = LINUX_SYSCALL(__NR_readlink, buf, link, sizeof(link) - 1);
	#else
		int rv = LINUX_SYSCALL(__NR_readlinkat, LINUX_AT_FDCWD, buf, link, sizeof(link) - 1);
	#endif
	if (rv < 0)
		return rv;

	// readlink does not report truncation separately. Never return a partial
	// path as a valid guest path when the input buffer may have filled.
	if ((unsigned int)rv >= sizeof(link) - 1)
		return -LINUX_ENAMETOOLONG;
	link[rv] = '\0';

	if (rv >= prefix_path_len && strncmp(link, prefix_path, prefix_path_len) == 0 &&
		(link[prefix_path_len] == '\0' || link[prefix_path_len] == '/'))
	{
		const char* guest_path = link + prefix_path_len;
		if (*guest_path == '\0')
			guest_path = "/";
		if (args->maxlen < strlen(guest_path) + 1)
			return -LINUX_ENAMETOOLONG;
		strcpy(args->path, guest_path);
	}
	else
	{
		if (args->maxlen < sizeof(EXIT_PATH) + rv)
			return -LINUX_ENAMETOOLONG;

		memcpy(args->path, EXIT_PATH, sizeof(EXIT_PATH) - 1);
		memcpy(args->path + sizeof(EXIT_PATH) - 1, link, rv+1);
	}

	return 0;
}

int vchroot_unexpand(struct vchroot_unexpand_args* args)
{
	if (memcmp(args->path, prefix_path, prefix_path_len) == 0)
	{
		int bytes = strlen(args->path + prefix_path_len) + 1;
		memmove(args->path, args->path + prefix_path_len, bytes);

		if (args->path[0] == '\0')
			strcpy(args->path, "/");
	}
	else
	{
		int bytes = strlen(args->path) + 1;
		memmove(args->path + sizeof(EXIT_PATH) - 1, args->path, bytes);
		memcpy(args->path, EXIT_PATH, sizeof(EXIT_PATH) - 1);
	}

	return 0;
}

#ifdef TEST
int main(int argc, const char** argv)
{
	if (argc != 3)
	{
		printf("Usage: vchroot_expand <vchroot_dir> <path_to_expand>\n");
		return 1;
	}

	int dfd = open(argv[1], O_RDONLY | O_DIRECTORY);
	__darling_vchroot(dfd);
	close(dfd);

	if (getenv("ICASE"))
		icase_enabled = true;

	struct vchroot_expand_args args;
	args.dfd = -100;
	strcpy(args.path, argv[2]);

	int rv = vchroot_expand(&args);
	printf("vchroot_expand() rv = %d\n", rv);
	printf("-> path = %s\n", args.path);

	if (rv == 0)
	{
		int fd = open(args.path, O_RDONLY);
		struct vchroot_fdpath_args fdpath_args;
		char buf[4096];

		fdpath_args.path = buf;
		fdpath_args.maxlen = sizeof(buf);
		fdpath_args.fd = fd;

		rv = vchroot_fdpath(&fdpath_args);

		printf("vchroot_fdpath() rv = %d\n", rv);
		if (rv >= 0)
			printf("-> path = %s\n", buf);

		close(fd);
	}
	
	return 0;
}
#endif
