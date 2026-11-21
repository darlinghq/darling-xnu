/*
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

// The kernel services that kern_synch.c expects from XNU, inside the process:
// locks, zones, time, threads and blocking. Every psynch call that runs here
// has all signals blocked except while it sleeps (see psynch_inproc.c), so
// nothing below can be re-entered by a signal handler of the same thread.

#include <pthread/tsd_private.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/common/simple.h>
#include <darling/emulation/common/tsd.h>
#include <darling/emulation/conversion/duct_errno.h>
#include <darling/emulation/conversion/signal/duct_signals.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/xnu_syscall/bsd/impl/bsdthread/bsdthread_register.h>
#include <darling/emulation/xnu_syscall/bsd/impl/bsdthread/pthread_canceled.h>

// last: see ksyn_kern.h
#include "ksyn_thread.h"

#define LINUX_FUTEX_WAIT         0
#define LINUX_FUTEX_WAKE         1
#define LINUX_FUTEX_WAIT_BITSET  9
#define LINUX_FUTEX_PRIVATE_FLAG 128
#define LINUX_FUTEX_BITSET_MATCH_ANY 0xffffffff
#define LINUX_CLOCK_MONOTONIC    1

#if defined(__x86_64__) || defined(__i386__)
#	define ksyn_cpu_relax() __builtin_ia32_pause()
#else
#	define ksyn_cpu_relax() do { } while (0)
#endif

// Linux's struct timespec for the syscalls used here (32-bit fields on i386)
struct ksyn_linux_timespec {
	long tv_sec;
	long tv_nsec;
};

// ---- futex helpers ---------------------------------------------------------------

static long ksyn_futex_wait(uint32_t* addr, uint32_t expected)
{
	return LINUX_SYSCALL(__NR_futex, addr, LINUX_FUTEX_WAIT | LINUX_FUTEX_PRIVATE_FLAG, expected, NULL, NULL, 0);
}

static void ksyn_futex_wake(uint32_t* addr, int count)
{
	LINUX_SYSCALL(__NR_futex, addr, LINUX_FUTEX_WAKE | LINUX_FUTEX_PRIVATE_FLAG, count, NULL, NULL, 0);
}

// ---- locks ---------------------------------------------------------------------

void ksyn_lock(ksyn_lock_t* lock)
{
	uint32_t expected = 0;

	if (__atomic_compare_exchange_n(&lock->word, &expected, 1, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
		return;

	// the sections these locks protect are short: spin a little before sleeping
	for (int i = 0; i < 100; i++)
	{
		ksyn_cpu_relax();
		expected = 0;
		if (__atomic_load_n(&lock->word, __ATOMIC_RELAXED) == 0 &&
			__atomic_compare_exchange_n(&lock->word, &expected, 1, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
			return;
	}

	while (__atomic_exchange_n(&lock->word, 2, __ATOMIC_ACQUIRE) != 0)
		ksyn_futex_wait(&lock->word, 2);
}

void ksyn_unlock(ksyn_lock_t* lock)
{
	if (__atomic_exchange_n(&lock->word, 0, __ATOMIC_RELEASE) == 2)
		ksyn_futex_wake(&lock->word, 1);
}

// msleep() and wakeup() are only used for the rare wait of ksyn_wqfind() for the
// last unlocker of a kwq that changes type, so a single channel serves all of them
static uint32_t ksyn_sleep_generation;

int msleep(void* chan, lck_mtx_t* mtx, int pri, const char* wmesg, struct timespec* ts)
{
	uint32_t generation = __atomic_load_n(&ksyn_sleep_generation, __ATOMIC_ACQUIRE);

	(void) chan;
	(void) wmesg;
	(void) ts;

	ksyn_unlock(mtx);
	ksyn_futex_wait(&ksyn_sleep_generation, generation);

	if (!(pri & PDROP))
		ksyn_lock(mtx);
	return 0;
}

void wakeup(void* chan)
{
	(void) chan;
	__atomic_add_fetch(&ksyn_sleep_generation, 1, __ATOMIC_RELEASE);
	ksyn_futex_wake(&ksyn_sleep_generation, 0x7fffffff);
}

// ---- memory ----------------------------------------------------------------------

static void* ksyn_pages(size_t size)
{
	long result;

#ifdef __NR_mmap2
	result = LINUX_SYSCALL(__NR_mmap2, NULL, size, 3 /* PROT_READ | PROT_WRITE */,
		0x22 /* MAP_PRIVATE | MAP_ANONYMOUS */, -1, 0);
#else
	result = LINUX_SYSCALL(__NR_mmap, NULL, size, 3 /* PROT_READ | PROT_WRITE */,
		0x22 /* MAP_PRIVATE | MAP_ANONYMOUS */, -1, 0);
#endif

	if (result < 0 && result > -4096)
		return NULL;
	return (void*) result;
}

void ksyn_bzero(void* ptr, size_t size)
{
	volatile char* p = ptr;
	while (size--)
		*p++ = 0;
}

#define KSYN_ZONE_CHUNK (64 * 1024)
#define KSYN_MAX_ZONES 4

struct ksyn_zone {
	ksyn_lock_t lock;
	size_t size;
	void* free_list;
	char* chunk;
	size_t chunk_left;
};

static struct ksyn_zone ksyn_zones[KSYN_MAX_ZONES];
static unsigned int ksyn_zone_count;

zone_t zinit(size_t size, size_t max, size_t alloc, const char* name)
{
	(void) max;
	(void) alloc;
	(void) name;

	if (ksyn_zone_count == KSYN_MAX_ZONES)
		ksyn_panic("psynch: out of zones");

	struct ksyn_zone* zone = &ksyn_zones[ksyn_zone_count++];
	zone->size = (size + 15) & ~(size_t) 15;
	return zone;
}

// unlike XNU's, the elements are zeroed
void* zalloc(zone_t zone)
{
	void* elem;

	ksyn_lock(&zone->lock);
	elem = zone->free_list;
	if (elem != NULL)
	{
		zone->free_list = *(void**) elem;
		ksyn_unlock(&zone->lock);
		ksyn_bzero(elem, zone->size);
		return elem;
	}

	if (zone->chunk_left < zone->size)
	{
		char* chunk = ksyn_pages(KSYN_ZONE_CHUNK);
		if (chunk == NULL)
			ksyn_panic("psynch: out of memory");
		zone->chunk = chunk;
		zone->chunk_left = KSYN_ZONE_CHUNK;
	}

	elem = zone->chunk;
	zone->chunk += zone->size;
	zone->chunk_left -= zone->size;
	ksyn_unlock(&zone->lock);

	// fresh pages are zero already
	return elem;
}

void zfree(zone_t zone, void* elem)
{
	ksyn_lock(&zone->lock);
	*(void**) elem = zone->free_list;
	zone->free_list = elem;
	ksyn_unlock(&zone->lock);
}

void* hashinit(int elements, int type, unsigned long* hashmask)
{
	unsigned long hashsize = 1;

	(void) type;
	while (hashsize * 2 <= (unsigned long) elements)
		hashsize *= 2;

	void* table = ksyn_pages(hashsize * sizeof(void*));
	if (table != NULL)
		*hashmask = hashsize - 1;
	return table;
}

void ksyn_free_hash(void* ptr)
{
	// only XNU's process exit frees the per-process table; that never happens here
	(void) ptr;
}

// ---- time ------------------------------------------------------------------------

static uint64_t ksyn_now(void)
{
	struct ksyn_linux_timespec ts;

	LINUX_SYSCALL(__NR_clock_gettime, LINUX_CLOCK_MONOTONIC, &ts);
	return (uint64_t) ts.tv_sec * NSEC_PER_SEC + (uint64_t) ts.tv_nsec;
}

void nanoseconds_to_absolutetime(uint64_t nanoseconds, uint64_t* result)
{
	*result = nanoseconds;
}

void clock_absolutetime_interval_to_deadline(uint64_t abstime, uint64_t* result)
{
	uint64_t now = ksyn_now();

	if (abstime > UINT64_MAX - now)
		*result = UINT64_MAX;
	else
		*result = now + abstime;
}

void microuptime(struct timeval* tvp)
{
	uint64_t now = ksyn_now();

	tvp->tv_sec = now / NSEC_PER_SEC;
	tvp->tv_usec = (now % NSEC_PER_SEC) / 1000;
}

uint64_t tvtoabstime(struct timeval* tvp)
{
	return (uint64_t) tvp->tv_sec * NSEC_PER_SEC + (uint64_t) tvp->tv_usec * 1000;
}

// The kext frees idle rwlock kwqs from a thread call some seconds after their last
// use. Here the deadline is noted, and the next psynch call after it does the cleanup.
static uint64_t ksyn_cleanup_deadline;
static uint32_t ksyn_cleanup_counter;

void thread_call_enter_delayed(thread_call_t call, uint64_t deadline)
{
	(void) call;
	__atomic_store_n(&ksyn_cleanup_deadline, deadline, __ATOMIC_RELAXED);
}

void ksyn_run_due_cleanup(void)
{
	uint64_t deadline = __atomic_load_n(&ksyn_cleanup_deadline, __ATOMIC_RELAXED);

	if (deadline == 0)
		return;

	// reading the clock is a syscall: look only every so often
	if ((__atomic_add_fetch(&ksyn_cleanup_counter, 1, __ATOMIC_RELAXED) & 63) != 0)
		return;

	if (ksyn_now() < deadline)
		return;

	if (!__atomic_compare_exchange_n(&ksyn_cleanup_deadline, &deadline, 0, false, __ATOMIC_RELAXED, __ATOMIC_RELAXED))
		return;

	psynch_wq_cleanup(NULL, NULL);
}

// ---- signals ---------------------------------------------------------------------

void ksyn_block_signals(ksyn_sigset_t* old)
{
	// all but the ones the kernel raises for a fault, which are synchronous
	linux_sigset_t block = ~0ull;
	block &= ~(1ull << (LINUX_SIGSEGV - 1));
	block &= ~(1ull << (LINUX_SIGBUS - 1));
	block &= ~(1ull << (LINUX_SIGILL - 1));
	block &= ~(1ull << (LINUX_SIGFPE - 1));
	block &= ~(1ull << (LINUX_SIGTRAP - 1));
	block &= ~(1ull << (LINUX_SIGSYS - 1));

	LINUX_SYSCALL(__NR_rt_sigprocmask, 0 /* LINUX_SIG_BLOCK */, &block, old, sizeof(linux_sigset_t));
}

void ksyn_restore_signals(const ksyn_sigset_t* old)
{
	LINUX_SYSCALL(__NR_rt_sigprocmask, 2 /* LINUX_SIG_SETMASK */, old, NULL, sizeof(linux_sigset_t));
}

// ---- threads ---------------------------------------------------------------------

static zone_t ksyn_thread_zone;
static zone_t ksyn_frame_zone;
static ksyn_lock_t ksyn_registry_lock;
static struct ksyn_thread* ksyn_registry;

// stands in for a thread that has never made an in-process psynch call: it waits on nothing
static struct ksyn_thread ksyn_absent_thread;

static lck_mtx_t ksyn_list_lock;
static void* ksyn_proc_pthhash;
static bool ksyn_initialized;
static ksyn_lock_t ksyn_init_lock;

bool ksyn_init(void)
{
	if (__atomic_load_n(&ksyn_initialized, __ATOMIC_ACQUIRE))
		return true;

	ksyn_lock(&ksyn_init_lock);
	if (!ksyn_initialized)
	{
		pthread_list_mlock = &ksyn_list_lock;
		ksyn_thread_zone = zinit(sizeof(struct ksyn_thread), 0, 0, "ksyn_thread");
		ksyn_frame_zone = zinit(sizeof(struct ksyn_frame), 0, 0, "ksyn_frame");
		psynch_zoneinit();
		_pth_proc_hashinit(current_proc());
		if (ksyn_proc_pthhash != NULL)
			__atomic_store_n(&ksyn_initialized, true, __ATOMIC_RELEASE);
	}
	ksyn_unlock(&ksyn_init_lock);

	return ksyn_initialized;
}

struct ksyn_thread* ksyn_thread_self(void)
{
	struct ksyn_thread* th = (struct ksyn_thread*) _pthread_getspecific_direct(__PTK_DARLING_PSYNCH);
	if (th != NULL)
		return th;

	ksyn_lock(&ksyn_registry_lock);
	for (th = ksyn_registry; th != NULL; th = th->registry_next)
	{
		if (!th->in_use)
			break;
	}
	if (th == NULL)
	{
		th = zalloc(ksyn_thread_zone);
		th->registry_next = ksyn_registry;
		ksyn_registry = th;
	}
	th->in_use = true;
	th->current = NULL;
	th->tid = LINUX_SYSCALL(__NR_gettid);
	th->port = (mach_port_name_t) (uintptr_t) _pthread_getspecific_direct(_PTHREAD_TSD_SLOT_MACH_THREAD_SELF);
	th->cancel_slot = (volatile uint8_t*) bsdthread_cancel_marked_slot();
	ksyn_unlock(&ksyn_registry_lock);

	_pthread_setspecific_direct(__PTK_DARLING_PSYNCH, (uintptr_t) th);
	return th;
}

void ksyn_thread_release_exiting(void)
{
	// The exiting thread's TSD may already be gone: libpthread wakes a joiner,
	// which frees the pthread_t, before the thread terminates. Find the thread's
	// state by its Linux thread ID instead.
	uint64_t tid = LINUX_SYSCALL(__NR_gettid);

	ksyn_lock(&ksyn_registry_lock);
	for (struct ksyn_thread* th = ksyn_registry; th != NULL; th = th->registry_next)
	{
		// a thread that left from inside a call (e.g. a signal handler that ended
		// the thread while it waited) may still have its wait element queued:
		// never reuse that
		if (th->in_use && th->tid == tid && th->current == NULL)
		{
			th->in_use = false;
			th->port = 0;
		}
	}
	ksyn_unlock(&ksyn_registry_lock);
}

struct ksyn_frame* ksyn_frame_push(struct ksyn_thread* th)
{
	struct ksyn_frame* frame;
	struct ksyn_frame* prev = th->current;
	struct ksyn_frame** link = prev ? &prev->next : &th->base;

	frame = *link;
	if (frame == NULL)
	{
		frame = zalloc(ksyn_frame_zone);
		frame->prev = prev;
		*link = frame;
	}

	frame->wait_state = KSYN_WAIT_IDLE;
	frame->sleeping = 0;
	frame->detached = false;
	frame->finish_jmp = NULL;
	frame->retval = 0;
	frame->error = 0;
	__atomic_store_n(&th->current, frame, __ATOMIC_RELEASE);
	return frame;
}

void ksyn_frame_pop(struct ksyn_thread* th, struct ksyn_frame* frame)
{
	// ksyn_signal_arrived() already took a detached frame out of the chain
	if (frame->detached)
	{
		zfree(ksyn_frame_zone, frame);
		return;
	}
	__atomic_store_n(&th->current, frame->prev, __ATOMIC_RELEASE);
}

// Takes the innermost frame out of the chain: the call it belongs to has been
// finished, and the frames of calls a handler makes now must not reuse it.
static void ksyn_frame_detach(struct ksyn_thread* th, struct ksyn_frame* frame)
{
	struct ksyn_frame** link = frame->prev ? &frame->prev->next : &th->base;

	*link = frame->next;
	if (frame->next != NULL)
		frame->next->prev = frame->prev;
	__atomic_store_n(&th->current, frame->prev, __ATOMIC_RELEASE);

	frame->prev = frame->next = NULL;
	frame->detached = true;
}

thread_t current_thread(void)
{
	return (thread_t) _pthread_getspecific_direct(__PTK_DARLING_PSYNCH);
}

struct uthread* current_uthread(void)
{
	return (struct uthread*) current_thread();
}

static struct ksyn_frame* ksyn_current_frame(void)
{
	return current_thread()->current;
}

uint64_t thread_tid(thread_t thread)
{
	return thread ? thread->tid : 0;
}

// Used by targeted condvar signals (pthread_cond_signal_thread_np). A port that is
// not a thread known here gets a stand-in that waits on nothing, so the kext
// turns the signal into a broadcast, as it does for a thread that is not waiting.
thread_t port_name_to_thread(mach_port_name_t name)
{
	struct ksyn_thread* th;

	ksyn_lock(&ksyn_registry_lock);
	for (th = ksyn_registry; th != NULL; th = th->registry_next)
	{
		if (th->in_use && th->port == name)
			break;
	}
	ksyn_unlock(&ksyn_registry_lock);

	return th ? th : &ksyn_absent_thread;
}

// ---- blocking ----------------------------------------------------------------------

void thread_block_parameter(thread_continue_t continuation, void* parameter)
{
	struct ksyn_frame* frame = ksyn_current_frame();
	struct ksyn_linux_timespec deadline;
	struct ksyn_linux_timespec* timeout = NULL;
	// psynch_cvwait is a cancellation point; the other waits are retried by libpthread
	bool cancelable = frame->block_hint == kThreadWaitPThreadCondVar;
	uint32_t state;
	wait_result_t result;

	if (frame->deadline != 0)
	{
		uint64_t secs = frame->deadline / NSEC_PER_SEC;

		deadline.tv_sec = (secs > (uint64_t) (~0ul >> 1)) ? (long) (~0ul >> 1) : (long) secs;
		deadline.tv_nsec = frame->deadline % NSEC_PER_SEC;
		timeout = &deadline;
	}

	frame->continuation = continuation;
	frame->parameter = parameter;

	// the caller's handlers may run while we sleep (we hold no locks now); one
	// that does finishes this wait first (ksyn_signal_arrived())
	__atomic_store_n(&frame->sleeping, 1, __ATOMIC_RELEASE);
	ksyn_restore_signals(&frame->sigmask);

	for (;;)
	{
		long ret;
		uint32_t next;

		if (__atomic_load_n(&frame->wait_state, __ATOMIC_ACQUIRE) != KSYN_WAIT_WAITING)
			break;

		// an absolute CLOCK_MONOTONIC deadline, which a restarted wait keeps
		if (cancelable)
		{
			ret = pthread_cancelable_syscall((long) &frame->wait_state,
				LINUX_FUTEX_WAIT_BITSET | LINUX_FUTEX_PRIVATE_FLAG, KSYN_WAIT_WAITING,
				(long) timeout, 0, LINUX_FUTEX_BITSET_MATCH_ANY, __NR_futex);
		}
		else
		{
			ret = LINUX_SYSCALL(__NR_futex, &frame->wait_state,
				LINUX_FUTEX_WAIT_BITSET | LINUX_FUTEX_PRIVATE_FLAG, KSYN_WAIT_WAITING,
				timeout, NULL, LINUX_FUTEX_BITSET_MATCH_ANY);
		}

		if (ret == -LINUX_ETIMEDOUT)
			next = KSYN_WAIT_TIMED_OUT;
		else if (ret == -LINUX_EINTR)
			next = KSYN_WAIT_INTERRUPTED;
		else
			continue;

		// unless a wakeup came first
		uint32_t expected = KSYN_WAIT_WAITING;
		if (__atomic_compare_exchange_n(&frame->wait_state, &expected, next, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE))
			break;
	}

	ksyn_block_signals(NULL);

	// (no handler can run from here on)
	if (__atomic_load_n(&frame->wait_state, __ATOMIC_ACQUIRE) == KSYN_WAIT_DONE)
		KSYN_LONGJMP(frame->return_jmp);
	__atomic_store_n(&frame->sleeping, 0, __ATOMIC_RELAXED);

	state = __atomic_load_n(&frame->wait_state, __ATOMIC_ACQUIRE);
	if (state == KSYN_WAIT_AWAKENED)
		result = THREAD_AWAKENED;
	else if (state == KSYN_WAIT_TIMED_OUT)
		result = THREAD_TIMED_OUT;
	else
		result = THREAD_INTERRUPTED;
	__atomic_store_n(&frame->wait_state, KSYN_WAIT_IDLE, __ATOMIC_RELAXED);

	continuation(parameter, result);
	__builtin_unreachable();
}

// XNU delivers a signal on the way back to user space: a psynch wait that the
// signal interrupts has ended (THREAD_INTERRUPTED: the thread is off the wait
// queue and the call returns EINTR) before the handler runs, and one that was
// woken or timed out just before has ended with that result. Here the guest
// handler runs inside the Linux signal handler, in the middle of the wait. So
// before it runs, the wait is finished here, on the handler's stack, exactly
// as thread_block_parameter() would finish it; the frame is taken out of the
// chain (calls the handler makes get frames of their own), and the interrupted
// thread_block_parameter() returns the stored result when the handler returns.
// A handler that never returns (siglongjmp) leaves nothing queued behind.
void ksyn_signal_arrived(struct ksyn_thread* th)
{
	struct ksyn_frame* frame = __atomic_load_n(&th->current, __ATOMIC_ACQUIRE);
	ksyn_sigset_t sigmask;
	wait_result_t result;
	uint32_t state;
	ksyn_jmp_buf finish_jmp;

	// signals that run guest handlers are blocked in psynch calls except while
	// they sleep, so only a sleeping frame can be the innermost one here
	if (frame == NULL || !__atomic_load_n(&frame->sleeping, __ATOMIC_ACQUIRE))
		return;

	ksyn_block_signals(&sigmask);
	__atomic_store_n(&frame->sleeping, 0, __ATOMIC_RELAXED);

	// as thread_block_parameter() on EINTR: interrupted, unless a wakeup came first
	uint32_t expected = KSYN_WAIT_WAITING;
	__atomic_compare_exchange_n(&frame->wait_state, &expected, KSYN_WAIT_INTERRUPTED, false,
		__ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE);

	state = __atomic_load_n(&frame->wait_state, __ATOMIC_ACQUIRE);
	if (state == KSYN_WAIT_AWAKENED)
		result = THREAD_AWAKENED;
	else if (state == KSYN_WAIT_TIMED_OUT)
		result = THREAD_TIMED_OUT;
	else
		result = THREAD_INTERRUPTED;
	__atomic_store_n(&frame->wait_state, KSYN_WAIT_IDLE, __ATOMIC_RELAXED);

	// the continuation ends in unix_syscall_return(), which comes back here
	frame->finish_jmp = finish_jmp;
	if (KSYN_SETJMP(finish_jmp) == 0)
	{
		frame->continuation(frame->parameter, result);
		__builtin_unreachable();
	}
	frame->finish_jmp = NULL;

	ksyn_frame_detach(th, frame);
	// a futex wait still in progress returns at once (or after the handler, on EINTR)
	__atomic_store_n(&frame->wait_state, KSYN_WAIT_DONE, __ATOMIC_RELEASE);

	ksyn_restore_signals(&sigmask);
}

// ---- pthread_kern callbacks ----------------------------------------------------------

static vm_map_t shim_current_map(void)
{
	return NULL;
}

static void* shim_get_bsdthread_info(thread_t th)
{
	return th;
}

static uint32_t shim_get_task_threadmax(void)
{
	// the value darlingserver's copy of the kext uses
	return 1024;
}

static void* shim_proc_get_pthhash(struct proc* p)
{
	(void) p;
	return ksyn_proc_pthhash;
}

static void shim_proc_set_pthhash(struct proc* p, void* ptr)
{
	(void) p;
	ksyn_proc_pthhash = ptr;
}

// no turnstiles: there is no priority inheritance to set up
static void shim_psynch_wait_cleanup(void)
{
}

static void shim_psynch_wait_complete(uintptr_t kwq, struct turnstile** tstore)
{
	(void) kwq;
	(void) tstore;
}

static void shim_psynch_wait_update_complete(struct turnstile* ts)
{
	(void) ts;
}

static void shim_psynch_wait_update_owner(uintptr_t kwq, thread_t owner, struct turnstile** tstore)
{
	(void) kwq;
	(void) owner;
	(void) tstore;
}

// called with the kwq locked, before thread_block_parameter()
static wait_result_t shim_psynch_wait_prepare(uintptr_t kwq, struct turnstile** tstore, thread_t owner,
		block_hint_t block_hint, uint64_t deadline)
{
	struct ksyn_frame* frame = ksyn_current_frame();

	(void) kwq;
	(void) tstore;
	(void) owner;

	frame->block_hint = block_hint;
	frame->deadline = deadline;
	__atomic_store_n(&frame->wait_state, KSYN_WAIT_WAITING, __ATOMIC_RELEASE);
	return THREAD_AWAKENED; // THREAD_WAITING in XNU; the kext ignores it
}

// called with the kwq locked; wakes the wait of that element, unless it has timed out or been interrupted
static kern_return_t shim_psynch_wait_wakeup(uintptr_t kwq, struct ksyn_waitq_element* kwe, struct turnstile** tstore)
{
	struct ksyn_frame* frame = (struct ksyn_frame*) kwe;
	uint32_t expected = KSYN_WAIT_WAITING;

	(void) kwq;
	(void) tstore;

	if (!__atomic_compare_exchange_n(&frame->wait_state, &expected, KSYN_WAIT_AWAKENED, false, __ATOMIC_ACQ_REL, __ATOMIC_RELAXED))
		return KERN_NOT_WAITING;

	ksyn_futex_wake(&frame->wait_state, 1);
	return KERN_SUCCESS;
}

// XNU acts on a pending cancel by aborting the next interruptible wait; the
// condvar wait sleeps through pthread_cancelable_syscall(), which does the same
static void shim___pthread_testcancel(int presyscall)
{
	(void) presyscall;
}

// owners only matter for turnstiles, which are not used here
static thread_t shim_task_findtid(task_t task, uint64_t tid)
{
	(void) task;
	(void) tid;
	return THREAD_NULL;
}

static void shim_thread_deallocate_safe(thread_t th)
{
	(void) th;
}

// ends the psynch call: back to its entry in psynch_inproc.c
static void __dead2 shim_unix_syscall_return(int error)
{
	struct ksyn_frame* frame = ksyn_current_frame();

	frame->error = error;
	if (frame->finish_jmp != NULL)
		KSYN_LONGJMP(frame->finish_jmp);
	KSYN_LONGJMP(frame->return_jmp);
}

static void* shim_uthread_get_uukwe(struct uthread* uth)
{
	struct ksyn_thread* th = (struct ksyn_thread*) uth;
	struct ksyn_frame* frame = __atomic_load_n(&th->current, __ATOMIC_ACQUIRE);

	return frame ? &frame->kwe : &th->idle_kwe;
}

// XNU skips a waiter that is marked for cancellation (and has it enabled, and
// has not acted on it yet) when it picks one to signal: that thread is about to
// leave its wait, and a signal it took would leave with it. darlingserver sets
// the first byte of the thread's __PTK_DARLING_CANCEL_MARKED slot when it marks
// it, and the thread clears it when it acts on the mark; the thread itself
// keeps the third byte set while it has cancellation disabled.
static int shim_uthread_is_cancelled(struct uthread* uth)
{
	struct ksyn_thread* th = (struct ksyn_thread*) uth;
	volatile uint8_t* slot = th->cancel_slot;

	if (slot == NULL)
		return 0;
	return __atomic_load_n(&slot[PTHREAD_CANCEL_MARKED_BYTE], __ATOMIC_ACQUIRE) != 0 &&
		__atomic_load_n(&slot[PTHREAD_CANCEL_DISABLED_BYTE], __ATOMIC_RELAXED) == 0;
}

static void shim_uthread_set_returnval(struct uthread* uth, int retval)
{
	struct ksyn_thread* th = (struct ksyn_thread*) uth;
	th->current->retval = retval;
}

// only the process-shared paths look up memory objects, and they never run here
static kern_return_t shim_vm_map_page_info(vm_map_t map, uint64_t offset, vm_page_info_flavor_t flavor,
		vm_page_info_t info, mach_msg_type_number_t* count)
{
	(void) map;
	(void) offset;
	(void) flavor;
	(void) info;
	(void) count;
	return KERN_INVALID_ARGUMENT;
}

static const struct pthread_callbacks_s ksyn_callbacks = {
	.current_map                 = shim_current_map,
	.get_bsdthread_info          = shim_get_bsdthread_info,
	.get_task_threadmax          = shim_get_task_threadmax,
	.proc_get_pthhash            = shim_proc_get_pthhash,
	.proc_set_pthhash            = shim_proc_set_pthhash,
	.psynch_wait_cleanup         = shim_psynch_wait_cleanup,
	.psynch_wait_complete        = shim_psynch_wait_complete,
	.psynch_wait_prepare         = shim_psynch_wait_prepare,
	.psynch_wait_update_complete = shim_psynch_wait_update_complete,
	.psynch_wait_update_owner    = shim_psynch_wait_update_owner,
	.psynch_wait_wakeup          = shim_psynch_wait_wakeup,
	.__pthread_testcancel        = shim___pthread_testcancel,
	.task_findtid                = shim_task_findtid,
	.thread_deallocate_safe      = shim_thread_deallocate_safe,
	.unix_syscall_return         = shim_unix_syscall_return,
	.uthread_get_uukwe           = shim_uthread_get_uukwe,
	.uthread_is_cancelled        = shim_uthread_is_cancelled,
	.uthread_set_returnval       = shim_uthread_set_returnval,
	.vm_map_page_info            = shim_vm_map_page_info,
};

pthread_callbacks_t pthread_kern = &ksyn_callbacks;

// ---- diagnostics -----------------------------------------------------------------------

void ksyn_panic(const char* message)
{
	__simple_printf("psynch: %s\n", message);
	__simple_abort();
}

// ---- fork ------------------------------------------------------------------------------

// The parent holds the locks of the shared structures across fork, so the child
// gets them in a consistent state. Signals are blocked by the caller.
void ksyn_prefork(void)
{
	if (!__atomic_load_n(&ksyn_initialized, __ATOMIC_ACQUIRE))
		return;

	ksyn_lock(&ksyn_list_lock);
	ksyn_lock(&ksyn_registry_lock);
	for (unsigned int i = 0; i < ksyn_zone_count; i++)
		ksyn_lock(&ksyn_zones[i].lock);
}

void ksyn_postfork_parent(void)
{
	if (!__atomic_load_n(&ksyn_initialized, __ATOMIC_ACQUIRE))
		return;

	for (unsigned int i = ksyn_zone_count; i > 0; i--)
		ksyn_unlock(&ksyn_zones[i - 1].lock);
	ksyn_unlock(&ksyn_registry_lock);
	ksyn_unlock(&ksyn_list_lock);
}

// Like a new XNU process, the child has no wait queues. The kwqs of the parent
// are left behind; only the forking thread exists in the child.
void ksyn_postfork_child(void)
{
	if (!__atomic_load_n(&ksyn_initialized, __ATOMIC_ACQUIRE))
		return;

	for (unsigned int i = 0; i < ksyn_zone_count; i++)
		ksyn_zones[i].lock.word = 0;
	ksyn_registry_lock.word = 0;
	ksyn_list_lock.word = 0;
	ksyn_sleep_generation = 0;
	ksyn_cleanup_deadline = 0;

	ksyn_postfork_child_reset();
	_pth_proc_hashinit(current_proc());

	struct ksyn_thread* self = (struct ksyn_thread*) _pthread_getspecific_direct(__PTK_DARLING_PSYNCH);
	for (struct ksyn_thread* th = ksyn_registry; th != NULL; th = th->registry_next)
	{
		if (th != self)
		{
			th->in_use = false;
			th->port = 0;
		}
	}
	// (its port is refreshed by its next call, after libpthread has updated it)
	if (self != NULL)
		self->tid = LINUX_SYSCALL(__NR_gettid);
}
