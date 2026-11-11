#include <darling/emulation/xnu_syscall/bsd/impl/bsdthread/workq_kernreturn.h>

#include <sys/errno.h>
#include <stddef.h>
#include <sys/queue.h>
#include <libsimple/lock.h>
#include <pthread/tsd_private.h>
#include <darling/emulation/common/tsd.h>

#define __PTHREAD_EXPOSE_INTERNALS__ 1
#include <pthread/priority_private.h>

#include <darling/emulation/xnu_syscall/bsd/impl/bsdthread/bsdthread_register.h>
#include <darling/emulation/xnu_syscall/bsd/impl/bsdthread/bsdthread_create.h>
#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/linux_premigration/ext/futex.h>
#include <darling/emulation/common/simple.h>
#include <darling/emulation/xnu_syscall/machdep/impl/tls.h>
#include <darling/emulation/xnu_syscall/mach/impl/mach_traps.h>
#include <darling/emulation/common/guarded/table.h>
#include <darling/emulation/linux_premigration/elfcalls_wrapper.h>

#include <elfcalls/threads.h>
#include <darlingserver/rpc.h>

// much easier to include than libpthread's `internal.h`
#include <darling/mldr/elfcalls/dthreads.h>

/*
 * Workqueue thread requests.
 *
 * libdispatch asks for worker threads with WQOPS_QUEUE_REQTHREADS (count in
 * `affinity`, pthread priority in `prio`) and never retries a request, so a
 * request must neither be dropped nor make the requesting thread wait for a
 * worker to become free: on macOS the kernel queues it and services it later.
 * Here every request is recorded as pending and then handed to an idle
 * (parked) worker or to a newly created one as far as the limits allow. What
 * cannot be serviced yet stays pending until a worker returns.
 *
 * Limits: constrained (non-overcommit) requests are serviced by at most
 * WQ_MAX_CONSTRAINED workers at a time. Overcommit requests and kevent
 * deliveries are not held back by that limit (a serial queue or an event must
 * get a thread even if every constrained worker is blocked), only by
 * WQ_MAX_THREADS running workers overall, the value macOS reports as
 * kern.wq_max_threads.
 *
 * Invariants, all protected by workq_lock:
 * - wq_running counts workers that have been given a request and have not
 *   returned it yet (including threads still being created for one);
 *   wq_running_constrained counts those whose request is constrained.
 *   They are incremented when a request is taken (take_request_locked) and
 *   decremented when the worker returns (WQOPS_THREAD_RETURN) or its thread
 *   could not be created.
 * - A worker on the parked list has no request. A waker removes it from the
 *   list, stores the request in its parked_thread record and then sets
 *   `state`; the parked worker only reads the record after seeing `state`
 *   set, and takes the lock once more before it leaves (its record lives on
 *   its own stack, which it is about to reuse), so the waker is done with the
 *   record by then.
 * - A request is in exactly one place: counted in wq_pending, linked on the
 *   kevent list, or held by one worker.
 *
 * Each worker remembers the flags it was started with in the
 * __PTK_DARLING_WORKQ_FLAGS TSD slot (to know which limit it counts against
 * when it returns) and the kevent request it is servicing, if any, in
 * __PTK_DARLING_WORKQ_KEVENT (so WQOPS_THREAD_KEVENT_RETURN completes the
 * right one even with several deliveries in flight).
 */
#define WQ_MAX_CONSTRAINED	64
#define WQ_MAX_THREADS		512

// Idle workers exit after this many seconds without work.
#define WQ_PARK_TIMEOUT		15


#define WQOPS_QUEUE_ADD 1
#define WQOPS_QUEUE_REMOVE 2
#define WQOPS_THREAD_RETURN 4
#define WQOPS_THREAD_SETCONC  8
#define WQOPS_THREAD_KEVENT_RETURN 0x40
#define WQOPS_QUEUE_NEWSPISUPP  0x10
#define WQOPS_QUEUE_REQTHREADS  0x20
#define WQOPS_QUEUE_REQTHREADS2    0x30
#define WQOPS_SET_EVENT_MANAGER_PRIORITY 0x80
#define WQOPS_SETUP_DISPATCH 0x400

// Flags for the newly spawned thread:
// WQ_FLAG_THREAD_NEWSPI
// WQ_FLAG_THREAD_REUSE (0 if thread is newly spawned)
// item is unused in "new SPI"

#define WQ_FLAG_THREAD_PRIO_SCHED    0x00008000
#define WQ_FLAG_THREAD_PRIO_QOS      0x00004000
#define WQ_FLAG_THREAD_PRIO_MASK     0x00000fff

#define WQ_FLAG_THREAD_OVERCOMMIT    0x00010000
#define WQ_FLAG_THREAD_REUSE         0x00020000
#define WQ_FLAG_THREAD_NEWSPI        0x00040000
#define WQ_FLAG_THREAD_KEVENT        0x00080000
#define WQ_FLAG_THREAD_EVENT_MANAGER 0x00100000
#define WQ_FLAG_THREAD_TSD_BASE_SET  0x00200000
#define WQ_FLAG_THREAD_WORKLOOP      0x00400000
#define WQ_FLAG_THREAD_OUTSIDEQOS    0x00800000

#define WORKQ_EXIT_THREAD_NKEVENT (-1)

struct timespec
{
	long tv_sec;
	long tv_nsec;
};

#ifndef VARIANT_DYLD

struct parked_thread
{
	// 0 while parked; set to 1 (under workq_lock, after `flags` and `kevent`
	// are filled in) when the thread is given a request. Futex word.
	int state;
	int flags;
	struct wq_kevent_data* kevent;
	TAILQ_ENTRY(parked_thread) entries;
};

static libsimple_lock_t workq_lock = LIBSIMPLE_LOCK_INITIALIZER;
static TAILQ_HEAD(parked_head, parked_thread) workq_parked = TAILQ_HEAD_INITIALIZER(workq_parked);

// Pending (not yet serviced) thread requests other than kevent deliveries,
// counted per thread QoS (0-7) and overcommit bit: index qos * 2 + overcommit.
#define WQ_BUCKETS 16
static unsigned int wq_pending[WQ_BUCKETS];

// Pending kevent deliveries, oldest first. Each wq_kevent_data belongs to a
// thread blocked in kevent_qos() until its delivery has been returned.
static struct wq_kevent_data* wq_kevent_head = NULL;
static struct wq_kevent_data** wq_kevent_tail = &wq_kevent_head;

static int wq_running = 0;
static int wq_running_constrained = 0;

static int priority_to_class(int prio);

static void rpc_guard(int fd) {
	guard_entry_options_t options;
	options.close = __dserver_close_socket;
	guard_table_add(fd, guard_flag_prevent_close | guard_flag_close_on_fork, &options);
};

static void rpc_unguard(int fd) {
	guard_table_remove(fd);
};

static const struct darling_thread_create_callbacks callbacks = {
	.thread_self_trap = &thread_self_trap_impl,
	.thread_set_tsd_base = &sys_thread_set_tsd_base,
	.rpc_guard = rpc_guard,
	.rpc_unguard = rpc_unguard,
};

static int extract_wq_flags(int priority) {
	int flags = 0;
	int qos = _pthread_priority_thread_qos(priority);

	flags = WQ_FLAG_THREAD_NEWSPI | qos | WQ_FLAG_THREAD_PRIO_QOS;

	if (priority & _PTHREAD_PRIORITY_OVERCOMMIT_FLAG) {
		flags |= WQ_FLAG_THREAD_OVERCOMMIT;
	}

	return flags;
};

static int bucket_of(int flags)
{
	int qos = flags & WQ_FLAG_THREAD_PRIO_MASK;
	if (qos > 7)
		qos = 7;
	return qos * 2 + ((flags & WQ_FLAG_THREAD_OVERCOMMIT) ? 1 : 0);
}

static int flags_of_bucket(int bucket)
{
	return WQ_FLAG_THREAD_NEWSPI | WQ_FLAG_THREAD_PRIO_QOS | (bucket / 2)
		| ((bucket & 1) ? WQ_FLAG_THREAD_OVERCOMMIT : 0);
}

static bool is_constrained(int flags)
{
	return !(flags & (WQ_FLAG_THREAD_OVERCOMMIT | WQ_FLAG_THREAD_KEVENT));
}

// What a worker is to do next.
struct wq_assignment
{
	int flags;
	struct wq_kevent_data* kevent;
};

// Takes the most urgent pending request that the limits allow to run now and
// counts it as running. Kevent deliveries come first (their requester is
// blocked waiting), then the highest QoS, overcommit before constrained at
// the same QoS. Called with workq_lock held.
static bool take_request_locked(struct wq_assignment* out)
{
	if (wq_running >= WQ_MAX_THREADS)
		return false;

	if (wq_kevent_head != NULL)
	{
		struct wq_kevent_data* ev = wq_kevent_head;

		wq_kevent_head = ev->next;
		if (wq_kevent_head == NULL)
			wq_kevent_tail = &wq_kevent_head;
		ev->next = NULL;

		out->flags = ev->flags;
		out->kevent = ev;
		wq_running++;
		return true;
	}

	for (int bucket = WQ_BUCKETS - 1; bucket >= 0; bucket--)
	{
		int flags;

		if (wq_pending[bucket] == 0)
			continue;

		flags = flags_of_bucket(bucket);
		if (is_constrained(flags) && wq_running_constrained >= WQ_MAX_CONSTRAINED)
			continue;

		wq_pending[bucket]--;
		out->flags = flags;
		out->kevent = NULL;
		wq_running++;
		if (is_constrained(flags))
			wq_running_constrained++;
		return true;
	}

	return false;
}

// Undoes take_request_locked() for a request no thread could be created for:
// the request goes back to the front of its queue. Called with workq_lock held.
static void untake_request_locked(const struct wq_assignment* a)
{
	wq_running--;
	if (is_constrained(a->flags))
		wq_running_constrained--;

	if (a->kevent != NULL)
	{
		a->kevent->next = wq_kevent_head;
		if (wq_kevent_head == NULL)
			wq_kevent_tail = &a->kevent->next;
		wq_kevent_head = a->kevent;
	}
	else
		wq_pending[bucket_of(a->flags)]++;
}

// Entry point of a newly created workqueue thread. For a kevent delivery,
// `item` is the wq_kevent_data; everything else gets no item.
static void workq_thread_start(void* self, int thread_port, void* stackaddr,
		void* item, int flags, int nevents)
{
	struct wq_kevent_data* ev = NULL;

	if (flags & WQ_FLAG_THREAD_KEVENT)
	{
		ev = (struct wq_kevent_data*) item;
		item = ev->events;
		nevents = ev->nevents;
	}

	_pthread_setspecific_direct(__PTK_DARLING_WORKQ_FLAGS, (uintptr_t) (flags & ~WQ_FLAG_THREAD_TSD_BASE_SET));
	_pthread_setspecific_direct(__PTK_DARLING_WORKQ_KEVENT, (uintptr_t) ev);

	wqueue_entry_point_wrapper(self, thread_port, stackaddr, item, flags, nevents);
}

static bool create_worker(const struct wq_assignment* a)
{
	void* pth = __darling_thread_create(512*1024, pthread_obj_size, workq_thread_start, 0,
			(uintptr_t) a->kevent, a->flags, 0, &callbacks, NULL);
	return pth != NULL;
}

// Hands pending requests to parked workers or to new threads while the
// limits allow. Called with workq_lock held; drops it around thread creation.
static void service_requests_locked(void)
{
	struct wq_assignment a;

	while (take_request_locked(&a))
	{
		struct parked_thread* t = TAILQ_FIRST(&workq_parked);

		if (t != NULL)
		{
			TAILQ_REMOVE(&workq_parked, t, entries);
			t->flags = a.flags;
			t->kevent = a.kevent;
			__atomic_store_n(&t->state, 1, __ATOMIC_RELEASE);
			// Woken while the lock is held: the thread takes the lock before
			// it leaves, so `t` stays valid until we are done with it.
			LINUX_SYSCALL(__NR_futex, &t->state, FUTEX_WAKE, 1, NULL, NULL, 0);
			continue;
		}

		libsimple_lock_unlock(&workq_lock);
		bool created = create_worker(&a);
		libsimple_lock_lock(&workq_lock);

		if (!created)
		{
			// Give the slot back. The request stays pending and is serviced
			// as soon as a worker returns or another request comes in.
			untake_request_locked(&a);
			break;
		}
	}
}

// Resets the workqueue in the child of fork(): only the forking thread exists
// there, so nobody is parked and nothing else runs. Pending requests belonged
// to the parent.
void workq_postfork_child(void)
{
	int flags = (int) (uintptr_t) _pthread_getspecific_direct(__PTK_DARLING_WORKQ_FLAGS);

	libsimple_lock_init(&workq_lock);
	TAILQ_INIT(&workq_parked);
	for (int i = 0; i < WQ_BUCKETS; i++)
		wq_pending[i] = 0;
	wq_kevent_head = NULL;
	wq_kevent_tail = &wq_kevent_head;

	// A worker that forked keeps running its request in the child and will
	// return it there.
	wq_running = (flags != 0) ? 1 : 0;
	wq_running_constrained = (flags != 0 && is_constrained(flags)) ? 1 : 0;
}

static long queue_requests(int flags, int count, struct wq_kevent_data* ev)
{
	libsimple_lock_lock(&workq_lock);

	if (ev != NULL)
	{
		ev->flags = flags | WQ_FLAG_THREAD_KEVENT;
		ev->next = NULL;
		*wq_kevent_tail = ev;
		wq_kevent_tail = &ev->next;
	}
	else
	{
		unsigned int* pending = &wq_pending[bucket_of(flags)];

		if (*pending > ~0u - (unsigned int) count)
			*pending = ~0u;
		else
			*pending += count;
	}

	service_requests_locked();
	libsimple_lock_unlock(&workq_lock);
	return 0;
}

// Completes the kevent delivery the calling worker was servicing, if any.
static void complete_kevent(struct kevent_qos_s* keventlist, int nkevents)
{
	struct wq_kevent_data* ev = (struct wq_kevent_data*) _pthread_getspecific_direct(__PTK_DARLING_WORKQ_KEVENT);

	if (ev == NULL)
		return;
	_pthread_setspecific_direct(__PTK_DARLING_WORKQ_KEVENT, 0);

	if (nkevents < 0)
		nkevents = 0;
	if (nkevents > ev->capacity)
		nkevents = ev->capacity;
	if (nkevents > 0 && keventlist != ev->events)
		memmove(ev->events, keventlist, nkevents * sizeof(struct kevent_qos_s));
	ev->nevents = nkevents;

	// The requester returns (and `ev` goes away with its stack) once this
	// is posted; nothing may touch `ev` afterwards.
	sem_up(&ev->sem);
}

// Re-enters the workqueue entry point on this thread's own stack.
static void __attribute__((noreturn)) resume_worker(int flags, struct wq_kevent_data* ev, bool terminating)
{
	int thread_self = _pthread_getspecific_direct(_PTHREAD_TSD_SLOT_MACH_THREAD_SELF);
	dthread_t dthread = _pthread_getspecific_direct(_PTHREAD_TSD_SLOT_PTHREAD_SELF);

	if (!terminating)
	{
		_pthread_setspecific_direct(__PTK_DARLING_WORKQ_FLAGS, (uintptr_t) flags);
		_pthread_setspecific_direct(__PTK_DARLING_WORKQ_KEVENT, (uintptr_t) ev);
	}

	// `_pthread_wqthread` needs valid flags even when it is only told to
	// exit (it derives the thread's priority from them).
	wqueue_entry_point_asm_jump(dthread, thread_self, dthread->stackbottom,
		(!terminating && ev != NULL) ? ev->events : NULL,
		flags | WQ_FLAG_THREAD_REUSE,
		terminating ? WORKQ_EXIT_THREAD_NKEVENT : (ev != NULL ? ev->nevents : 0)
	);

	__builtin_unreachable();
}

// The calling worker has finished its request: run the next pending one,
// or park until one arrives, or exit after WQ_PARK_TIMEOUT idle seconds.
static void __attribute__((noreturn)) worker_return(void)
{
	int my_flags = (int) (uintptr_t) _pthread_getspecific_direct(__PTK_DARLING_WORKQ_FLAGS);
	struct wq_assignment next;
	struct parked_thread me;
	struct timespec ts = { WQ_PARK_TIMEOUT, 0 };

	if (my_flags == 0)
	{
		// Not started by us: derive valid flags so the thread can at least
		// exit cleanly through `_pthread_wqthread`.
		my_flags = extract_wq_flags(_pthread_getspecific_direct(_PTHREAD_TSD_SLOT_PTHREAD_QOS_CLASS));
		resume_worker(my_flags, NULL, true);
	}

	libsimple_lock_lock(&workq_lock);

	wq_running--;
	if (is_constrained(my_flags))
		wq_running_constrained--;

	// Our slot is free. If a request was waiting for one, this thread
	// services it without parking; the others can only be waiting for the
	// constrained limit, which only a returning worker frees.
	if (take_request_locked(&next))
	{
		libsimple_lock_unlock(&workq_lock);
		resume_worker(next.flags, next.kevent, false);
	}

	me.state = 0;
	me.flags = 0;
	me.kevent = NULL;
	TAILQ_INSERT_HEAD(&workq_parked, &me, entries);

	libsimple_lock_unlock(&workq_lock);

	while (__atomic_load_n(&me.state, __ATOMIC_ACQUIRE) == 0)
	{
		long result = LINUX_SYSCALL(__NR_futex, &me.state, FUTEX_WAIT, 0, &ts, NULL, 0);

		// Linux's ETIMEDOUT. EAGAIN means we were just woken, EINTR is a signal
		// (which starts the timeout over; at worst the thread retires late).
		if (result == -110)
			break;
	}

	libsimple_lock_lock(&workq_lock);

	if (me.state == 0)
	{
		// Timed out with nothing assigned: leave the pool.
		TAILQ_REMOVE(&workq_parked, &me, entries);
		libsimple_lock_unlock(&workq_lock);
		resume_worker(my_flags & ~WQ_FLAG_THREAD_KEVENT, NULL, true);
	}

	// Assigned. The waker removed us from the list and holds no reference to
	// `me` once it has dropped the lock, which it has since we got it.
	next.flags = me.flags;
	next.kevent = me.kevent;
	libsimple_lock_unlock(&workq_lock);

	resume_worker(next.flags, next.kevent, false);
}

#else // VARIANT_DYLD

void workq_postfork_child(void)
{
}

#endif // VARIANT_DYLD

long sys_workq_kernreturn(int options, void* item, int affinity, int prio)
{
#ifndef VARIANT_DYLD
	struct wq_kevent_data* wq_event = NULL;

	// item is only used with WQOPS_QUEUE_ADD
	switch (options)
	{
		case WQOPS_THREAD_KEVENT_RETURN:
			complete_kevent((struct kevent_qos_s*) item, affinity);
			// fall through
		case WQOPS_THREAD_RETURN:
			// The thread has completed its job and can be either recycled
			// or terminated.
			worker_return();
		case WQOPS_SET_EVENT_MANAGER_PRIORITY:
		case WQOPS_QUEUE_NEWSPISUPP:
			return 0;
		case WQOPS_QUEUE_REQTHREAD_FOR_KEVENT:
		{
			wq_event = (struct wq_kevent_data*) item;
			affinity = 1;
		}
		case WQOPS_QUEUE_REQTHREADS2: // with prop bucket
		case WQOPS_QUEUE_REQTHREADS:
		{
			// affinity contains thread count
			if (affinity <= 0)
				return 0;

			// Never waits for a worker: what cannot run now stays pending.
			return queue_requests(extract_wq_flags(prio), affinity, wq_event);
		}
		case WQOPS_SETUP_DISPATCH: {
			// TODO: actually do something with the info passed in
			// XNU just tacks it onto the process its called for and uses it later when someone asks for it

			/*
			struct workq_dispatch_config cfg = {0};

			// `affinity` contains the size of the config structure passed in
			memcpy(&cfg, item, (affinity < sizeof(struct workq_dispatch_config)) ? affinity : sizeof(struct workq_dispatch_config));
			*/

			return 0;
		} break;
		default:
			return -ENOTSUP;
	}
#else
	return -ENOSYS;
#endif
}

// A counting semaphore: *sem is the number of tokens, and waiters sleep while it's zero.
// Returns 0 only if the timeout really expired, as the caller then assumes nobody woke it up.
// An interrupted wait starts the timeout over, which at worst retires an idle thread late.
int sem_down(int* sem, int timeout)
{
	struct timespec ts = { timeout, 0 };

	while (true)
	{
		int value = __atomic_load_n(sem, __ATOMIC_ACQUIRE);

		while (value > 0)
		{
			if (__atomic_compare_exchange_n(sem, &value, value - 1, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
				return 1;
		}

		long result = LINUX_SYSCALL(__NR_futex, sem, FUTEX_WAIT, 0, (timeout != -1) ? &ts : NULL, NULL, 0);

		// Linux's ETIMEDOUT. EAGAIN means a token just arrived, and EINTR is just a signal.
		if (result == -110)
			return 0;
	}
}

void sem_up(int* sem)
{
	__atomic_fetch_add(sem, 1, __ATOMIC_RELEASE);
	LINUX_SYSCALL(__NR_futex, sem, FUTEX_WAKE, 1, NULL, NULL, 0);
}

#define __PTHREAD_PRIORITY_CBIT_USER_INTERACTIVE 0x20
#define __PTHREAD_PRIORITY_CBIT_USER_INITIATED 0x10
#define __PTHREAD_PRIORITY_CBIT_DEFAULT 0x8
#define __PTHREAD_PRIORITY_CBIT_UTILITY 0x4
#define __PTHREAD_PRIORITY_CBIT_BACKGROUND 0x2
#define __PTHREAD_PRIORITY_CBIT_MAINTENANCE 0x1
#define __PTHREAD_PRIORITY_CBIT_UNSPECIFIED 0x0

#define QOS_CLASS_USER_INTERACTIVE 0x21
#define QOS_CLASS_USER_INITIATED 0x19
#define QOS_CLASS_DEFAULT 0x15
#define QOS_CLASS_UTILITY 0x11
#define QOS_CLASS_BACKGROUND 0x09
#define QOS_CLASS_MAINTENANCE 0x05
#define QOS_CLASS_UNSPECIFIED 0x0

static int priority_to_class(int prio)
{
	int dec_prio = (prio & 0xff00) >> 8;
	switch (dec_prio)
	{
		case __PTHREAD_PRIORITY_CBIT_USER_INTERACTIVE:
			return __PTHREAD_PRIORITY_CBIT_MAINTENANCE;
		case __PTHREAD_PRIORITY_CBIT_USER_INITIATED:
			return QOS_CLASS_USER_INITIATED;
		case __PTHREAD_PRIORITY_CBIT_DEFAULT:
			return QOS_CLASS_DEFAULT;
		case __PTHREAD_PRIORITY_CBIT_UTILITY:
			return QOS_CLASS_UTILITY;
		case __PTHREAD_PRIORITY_CBIT_BACKGROUND:
			return QOS_CLASS_BACKGROUND;
		case __PTHREAD_PRIORITY_CBIT_MAINTENANCE:
			return QOS_CLASS_MAINTENANCE;
		default:
			return QOS_CLASS_UNSPECIFIED;
	}
}
