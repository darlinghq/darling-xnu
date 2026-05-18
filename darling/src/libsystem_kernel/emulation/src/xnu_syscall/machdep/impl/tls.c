#include <darling/emulation/xnu_syscall/machdep/impl/tls.h>

#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

#define ARCH_SET_GS	0x1001

#if defined(__aarch64__)
// On ARM64 Linux, there's only one user TLS register (TPIDR_EL0), already
// used by Linux pthreads. We cannot use a __thread variable here because:
//   1. tls.c.o gets linked into dyld (the loader executable, via libsystem_kernel_static64.a)
//   2. The dyld executable bootstraps before TLV support, so __thread variables
//      compiled for Darwin emit unresolved __tlv_bootstrap references at link time.
// Instead, we use a small open-addressed hash table keyed by Linux tid (from gettid).
// This is slower than a register but works correctly without TLV.

#define TSD_TABLE_SIZE 1024  /* power of two; > expected max thread count */

struct tsd_entry {
	long tid;       /* Linux tid; 0 means empty slot */
	void* base;
};

static struct tsd_entry tsd_table[TSD_TABLE_SIZE];

/* Fallback zero TSD page returned before pthread/dyld has set up a real one.
 * Some early-init code in dyld (e.g. __os_once) dereferences offsets off the
 * TSD base before _pthread_set_self_dyld() has called us. Returning NULL there
 * causes a NULL-deref SIGSEGV; returning a zero page makes those reads return 0
 * and lets initialization proceed until pthread installs the real TSD. The
 * page is intentionally 4K-aligned and big enough for the largest TSD index. */
static void* __attribute__((aligned(4096))) tsd_zero_page[4096 / sizeof(void*)];

static inline unsigned int tsd_hash(long tid)
{
	/* Simple multiplicative hash; collisions resolved by linear probing. */
	return (unsigned int)((tid * 2654435761u) & (TSD_TABLE_SIZE - 1));
}

/* Use the Linux thread pointer (TPIDR_EL0) as the thread identity. It's a
 * unique address per pthread, readable without a syscall, and stable across
 * the thread's lifetime. Using gettid() here would call __NR_gettid on every
 * TSD access (~ tens of thousands per second), wasting CPU. */
static inline long current_tid(void)
{
	return (long)__builtin_thread_pointer();
}

/* Single-entry cache. gettid() is a Linux syscall and we are called *very*
 * frequently (every errno read, every pthread_self()), so an uncached path
 * burns the process at ~100% CPU on a syscall trampoline. The cache is
 * deliberately not __thread (we cannot rely on TLV in dyld) — it's a regular
 * global. Concurrent readers/writers race harmlessly: a stale tid means we
 * fall through to the slow path and refresh. */
static volatile long  tsd_cache_tid;
static void* volatile tsd_cache_base = (void*)0; /* unused before first set */

__attribute__((visibility("default")))
void* sys_thread_get_tsd_base(void)
{
	long tid = current_tid();
	/* Fast path: hits when the same thread keeps calling us (common
	 * during single-threaded dyld init and launchd setup). */
	if (tid == tsd_cache_tid && tsd_cache_base != (void*)0)
		return tsd_cache_base;

	unsigned int i = tsd_hash(tid);
	for (unsigned int step = 0; step < TSD_TABLE_SIZE; step++)
	{
		struct tsd_entry* e = &tsd_table[(i + step) & (TSD_TABLE_SIZE - 1)];
		if (e->tid == tid) {
			tsd_cache_tid = tid;
			tsd_cache_base = e->base;
			return e->base;
		}
		if (e->tid == 0)
			break;
	}
	return tsd_zero_page;
}

static void tsd_set(long tid, void* base)
{
	unsigned int i = tsd_hash(tid);
	for (unsigned int step = 0; step < TSD_TABLE_SIZE; step++)
	{
		struct tsd_entry* e = &tsd_table[(i + step) & (TSD_TABLE_SIZE - 1)];
		if (e->tid == 0 || e->tid == tid)
		{
			e->base = base;
			e->tid = tid;
			/* Warm the fast path so the next get on this tid is O(1). */
			tsd_cache_base = base;
			tsd_cache_tid = tid;
			return;
		}
	}
	/* Table full; this would only happen with > 1024 concurrent threads.
	 * Real Darwin TSD never spills, so behavior is undefined here. */
}
#endif

void sys_thread_set_tsd_base(void* ptr, int unk)
{
#ifdef __x86_64__
	LINUX_SYSCALL(__NR_arch_prctl, ARCH_SET_GS, ptr);
#elif defined(__i386__)
	struct user_desc
	{
		unsigned int  entry_number;
		unsigned long base_addr;
		unsigned int  limit;
		unsigned int  seg_32bit:1;
		unsigned int  contents:2;
		unsigned int  read_exec_only:1;
		unsigned int  limit_in_pages:1;
		unsigned int  seg_not_present:1;
		unsigned int  useable:1;
	};

	struct user_desc desc;
	static int entry_number = -1;

	desc.base_addr = (unsigned long) ptr;
	desc.limit = 4096;
	desc.seg_32bit = 1;
	desc.contents = 0;
	desc.read_exec_only = 0;
	desc.limit_in_pages = 1;
	desc.seg_not_present = 0;
	desc.useable = 1;
	desc.entry_number = entry_number;

	LINUX_SYSCALL(__NR_set_thread_area, &desc);

	entry_number = desc.entry_number;
	__asm__ ("movl %0, %%fs" :: "r" (desc.entry_number*8 + 3));
#elif defined(__aarch64__)
	tsd_set(current_tid(), ptr);
#endif
}
