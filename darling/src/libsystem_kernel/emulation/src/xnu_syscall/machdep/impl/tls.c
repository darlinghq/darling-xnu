#include <darling/emulation/xnu_syscall/machdep/impl/tls.h>
#include <darling/emulation/conversion/duct_errno.h>
#include <stddef.h>
#include <stdint.h>

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

#define TSD_TABLE_BITS 10
#define TSD_TABLE_SIZE (1u << TSD_TABLE_BITS)  /* 1024; > expected max thread count */

struct tsd_entry {
	long tid;       /* Linux tid; 0 means empty slot */
	void* base;
};

static struct tsd_entry tsd_table[TSD_TABLE_SIZE];

/* Optional native-loader-owned ELF TLS slot. Older loaders omit the apple
 * metadata, retaining table lookup and the cached-instruction trap fallback. */
static unsigned long native_tsd_slot_offset = ~0UL;
unsigned long sys_thread_get_native_tsd_slot_offset(void) {
    return native_tsd_slot_offset;
}
static void publish_native_tsd_slot(void* base) {
    if (native_tsd_slot_offset != ~0UL)
        __atomic_store_n((void**)((uintptr_t)__builtin_thread_pointer() + native_tsd_slot_offset), base, __ATOMIC_RELEASE);
}
void sys_thread_set_native_tsd_slot_offset(unsigned long offset) {
    // The cache's LDR unsigned immediate is an aligned 12-bit count of words.
    if (offset < 16 || offset > 32760 || (offset & 7))
        return;
    native_tsd_slot_offset = offset;
    publish_native_tsd_slot(sys_thread_get_tsd_base());
}


/* Fallback zero TSD page returned before pthread/dyld has set up a real one.
 * Some early-init code in dyld (e.g. __os_once) dereferences offsets off the
 * TSD base before _pthread_set_self_dyld() has called us. Returning NULL there
 * causes a NULL-deref SIGSEGV; returning a zero page makes those reads return 0
 * and lets initialization proceed until pthread installs the real TSD. The
 * page is intentionally 4K-aligned and big enough for the largest TSD index. */
static void* __attribute__((aligned(4096))) tsd_zero_page[4096 / sizeof(void*)];

static inline unsigned int tsd_hash(long tid)
{
	/* Multiplicative (Fibonacci) hash using the top bits of the product; collisions
	 * are resolved by linear probing. The low bits of the product would only depend on
	 * the low bits of the thread pointer, and those are the same for every thread with
	 * the same stack size (glibc puts it at a fixed offset in a page-aligned mapping),
	 * which piled all such threads into one bucket. */
	return (unsigned int)(((unsigned long)tid * 0x9E3779B97F4A7C15ul) >> (64 - TSD_TABLE_BITS));
}

/* Use the Linux thread pointer (TPIDR_EL0) as the thread identity. It's a
 * unique address per pthread, readable without a syscall, and stable across
 * the thread's lifetime. Using gettid() here would call __NR_gettid on every
 * TSD access (~ tens of thousands per second), wasting CPU. */
static inline long current_tid(void)
{
	return (long)__builtin_thread_pointer();
}

/* There is deliberately no global "last lookup" cache: a tid/base pair kept in
 * two shared variables can be torn by another thread between the two reads,
 * handing this thread another thread's TSD (errno, pthread_self, pthread keys,
 * thread-local variables). current_tid() costs no syscall, so the table lookup
 * (usually one probe) is cheap enough on its own.
 *
 * Each entry is only ever read and written by its own thread; the only shared
 * step is claiming an empty slot, which uses compare-and-swap. */
__attribute__((visibility("default")))
void* sys_thread_get_tsd_base(void)
{
	long tid = current_tid();
	unsigned int i = tsd_hash(tid);
	for (unsigned int step = 0; step < TSD_TABLE_SIZE; step++)
	{
		struct tsd_entry* e = &tsd_table[(i + step) & (TSD_TABLE_SIZE - 1)];
		long entry_tid = e->tid;
		if (entry_tid == tid) {
			/* base is still NULL if this thread is inside tsd_set() between claiming
			 * the slot and storing it (e.g. a signal handler running in between). */
			void* base = e->base;
			return base ? base : tsd_zero_page;
		}
		if (entry_tid == 0)
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
		if (e->tid == tid || __sync_bool_compare_and_swap(&e->tid, 0, tid))
		{
			e->base = base;
			return;
		}
	}
	/* Table full; this would only happen with > 1024 concurrent threads.
	 * Real Darwin TSD never spills, so behavior is undefined here. */
}

/* Whether the calling thread has a TSD base registered in this copy of the table
 * (dyld links a separate static copy). Only lkm.c uses it, so it stays out of
 * libsystem_kernel's exports. */
bool sys_thread_has_tsd_base(void)
{
	long tid = current_tid();
	unsigned int i = tsd_hash(tid);
	for (unsigned int step = 0; step < TSD_TABLE_SIZE; step++)
	{
		long entry_tid = tsd_table[(i + step) & (TSD_TABLE_SIZE - 1)].tid;
		if (entry_tid == tid)
			return true;
		if (entry_tid == 0)
			break;
	}
	return false;
}

/* TPIDR_EL0-relative offset of rseq.cpu_id (cpu_id_start_offset + 4):
 *   0  = not yet initialized
 *  -1  = initialized, no glibc rseq area (use per-thread rseq_table fallback)
 * else = valid offset from __builtin_thread_pointer() to int32_t cpu_id */
static intptr_t rseq_cpu_id_offset = 0;

void sys_thread_set_rseq_cpu_offset(intptr_t cpu_id_start_offset)
{
	intptr_t val = (cpu_id_start_offset == -1) ? -1 : (cpu_id_start_offset + 4);
	__atomic_store_n(&rseq_cpu_id_offset, val, __ATOMIC_RELEASE);
}

__attribute__((visibility("default")))
intptr_t sys_thread_get_rseq_cpu_id_offset(void)
{
	return __atomic_load_n(&rseq_cpu_id_offset, __ATOMIC_ACQUIRE);
}

/* Fallback per-thread rseq registration when glibc/mldr did not provide
 * darling_rseq_cpu_offset=. Uses __NR_rseq (293 on arm64).
 * state: 0 = uninit, 1 = registering, 2 = active, -1 = failed.
 * tid:   0 = empty, -1 = tombstone (released on thread exit). */
#define DARLING_RSEQ_SIG 0xd428bc00u
#define DARLING_RSEQ_CPU_ID_UNINITIALIZED (-1)

enum {
	RSEQ_STATE_UNINIT = 0,
	RSEQ_STATE_REGISTERING = 1,
	RSEQ_STATE_ACTIVE = 2,
	RSEQ_STATE_FAILED = -1,
};

struct darling_rseq_area {
	uint32_t cpu_id_start;
	int32_t cpu_id;
	uint64_t rseq_cs;
	uint32_t flags;
	uint32_t node_id;
	uint32_t mm_cid;
	uint32_t pad;
} __attribute__((aligned(32)));

struct rseq_entry {
	long tid;
	int state;
	struct darling_rseq_area rseq;
};

static struct rseq_entry rseq_table[TSD_TABLE_SIZE];

static struct rseq_entry* rseq_find_or_claim(long tid)
{
	unsigned int i = tsd_hash(tid);
	struct rseq_entry* tombstone = NULL;
	for (unsigned int step = 0; step < TSD_TABLE_SIZE; step++) {
		struct rseq_entry* e = &rseq_table[(i + step) & (TSD_TABLE_SIZE - 1)];
		long entry_tid = __atomic_load_n(&e->tid, __ATOMIC_ACQUIRE);
		if (entry_tid == tid)
			return e;
		if (entry_tid == -1 && tombstone == NULL) {
			tombstone = e;
			continue;
		}
		if (entry_tid == 0) {
			if (tombstone != NULL &&
			    __sync_bool_compare_and_swap(&tombstone->tid, -1, tid))
				return tombstone;
			if (__sync_bool_compare_and_swap(&e->tid, 0, tid))
				return e;
			if (__atomic_load_n(&e->tid, __ATOMIC_ACQUIRE) == tid)
				return e;
		}
	}
	if (tombstone != NULL &&
	    __sync_bool_compare_and_swap(&tombstone->tid, -1, tid))
		return tombstone;
	return NULL;
}

__attribute__((visibility("default")))
int sys_thread_rseq_cpu_number(void)
{
	if (__atomic_load_n(&rseq_cpu_id_offset, __ATOMIC_RELAXED) == -2)
		return -1;

	long tid = current_tid();
	struct rseq_entry* e = rseq_find_or_claim(tid);
	if (e == NULL)
		return -1;

	int state = __atomic_load_n(&e->state, __ATOMIC_ACQUIRE);
	if (state == RSEQ_STATE_ACTIVE) {
		int32_t cpu = __atomic_load_n(&e->rseq.cpu_id, __ATOMIC_RELAXED);
		return (cpu >= 0) ? cpu : -1;
	}
	if (state == RSEQ_STATE_FAILED || state == RSEQ_STATE_REGISTERING)
		return -1;

	if (__sync_bool_compare_and_swap(&e->state, RSEQ_STATE_UNINIT, RSEQ_STATE_REGISTERING)) {
		e->rseq.cpu_id = DARLING_RSEQ_CPU_ID_UNINITIALIZED;
		long rc = LINUX_SYSCALL(__NR_rseq, &e->rseq, 32, 0, DARLING_RSEQ_SIG);
		if (rc < 0) {
			if (rc == -LINUX_ENOSYS)
				__atomic_store_n(&rseq_cpu_id_offset, -2, __ATOMIC_RELEASE);
			__atomic_store_n(&e->state, RSEQ_STATE_FAILED, __ATOMIC_RELEASE);
			return -1;
		}
		__atomic_store_n(&e->state, RSEQ_STATE_ACTIVE, __ATOMIC_RELEASE);
		int32_t cpu = __atomic_load_n(&e->rseq.cpu_id, __ATOMIC_RELAXED);
		return (cpu >= 0) ? cpu : -1;
	}
	return -1;
}

void sys_thread_rseq_cleanup(void)
{
	long tid = current_tid();
	unsigned int i = tsd_hash(tid);
	for (unsigned int step = 0; step < TSD_TABLE_SIZE; step++) {
		struct rseq_entry* e = &rseq_table[(i + step) & (TSD_TABLE_SIZE - 1)];
		long entry_tid = __atomic_load_n(&e->tid, __ATOMIC_ACQUIRE);
		if (entry_tid == tid) {
			int prev_state = __atomic_exchange_n(&e->state, RSEQ_STATE_UNINIT, __ATOMIC_ACQ_REL);
			if (prev_state == RSEQ_STATE_ACTIVE)
				LINUX_SYSCALL(__NR_rseq, &e->rseq, 32, 1 /* RSEQ_FLAG_UNREGISTER */, DARLING_RSEQ_SIG);
			e->rseq = (struct darling_rseq_area){0};
			__atomic_store_n(&e->tid, -1, __ATOMIC_RELEASE);
			return;
		}
		if (entry_tid == 0)
			return;
	}
}

void sys_thread_rseq_postfork_child(void)
{
	long self_tid = current_tid();
	for (unsigned int i = 0; i < TSD_TABLE_SIZE; i++) {
		if (rseq_table[i].tid == self_tid &&
		    (rseq_table[i].state == RSEQ_STATE_ACTIVE ||
		     rseq_table[i].state == RSEQ_STATE_REGISTERING)) {
			LINUX_SYSCALL(__NR_rseq, &rseq_table[i].rseq, 32, 1 /* RSEQ_FLAG_UNREGISTER */, DARLING_RSEQ_SIG);
		}
		rseq_table[i].tid = 0;
		rseq_table[i].state = RSEQ_STATE_UNINIT;
		rseq_table[i].rseq = (struct darling_rseq_area){0};
	}
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
	publish_native_tsd_slot(ptr);
#endif
}
