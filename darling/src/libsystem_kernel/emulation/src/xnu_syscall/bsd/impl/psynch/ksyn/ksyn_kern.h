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

// The XNU kernel interfaces that the pthread kext's synchronizer (kern_synch.c)
// uses, implemented inside the process (ksyn_kern.c). kern_synch.c serves the
// psynch calls on process-private objects; see psynch_inproc.c.

#ifndef KSYN_KERN_H
#define KSYN_KERN_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/cdefs.h>
#include <sys/errno.h>
#include <sys/queue.h>
#include <sys/time.h>

// ---- basic kernel types --------------------------------------------------
//
// Users of this header that also include Mach or Darling headers include them
// first: the kernel's meaning of these names (e.g. thread_t) differs from the
// user-space one, so they are macros for ksyn_ types from here on.

#define kern_return_t ksyn_kern_return_t
#define wait_result_t ksyn_wait_result_t
#define mach_port_name_t ksyn_mach_port_name_t
#define mach_msg_type_number_t ksyn_mach_msg_type_number_t
#define thread_t ksyn_thread_t
#define proc_t ksyn_proc_t
#define task_t ksyn_task_t
#define vm_map_t ksyn_vm_map_t
#define vm_page_info_basic_data_t ksyn_vm_page_info_basic_data_t
#define vm_page_info_t ksyn_vm_page_info_t
#define vm_page_info_flavor_t ksyn_vm_page_info_flavor_t
#define block_hint_t ksyn_block_hint_t
#define thread_continue_t ksyn_thread_continue_t
#define zone_t ksyn_zone_t
#define thread_call_t ksyn_thread_call_t
#define lck_spin_t ksyn_lck_spin_t
#define lck_mtx_t ksyn_lck_mtx_t
#define lck_grp_t ksyn_lck_grp_t
#define lck_attr_t ksyn_lck_attr_t

// the kernel functions implemented here get names of their own in the library
#define msleep ksyn_msleep
#define wakeup ksyn_wakeup
#define zinit ksyn_zinit
#define zalloc ksyn_zalloc
#define zfree ksyn_zfree
#define hashinit ksyn_hashinit
#define current_thread ksyn_current_thread
#define current_uthread ksyn_current_uthread
#define thread_tid ksyn_thread_tid
#define port_name_to_thread ksyn_port_name_to_thread
#define thread_block_parameter ksyn_thread_block_parameter
#define nanoseconds_to_absolutetime ksyn_nanoseconds_to_absolutetime
#define clock_absolutetime_interval_to_deadline ksyn_clock_absolutetime_interval_to_deadline
#define microuptime ksyn_microuptime
#define tvtoabstime ksyn_tvtoabstime
#define thread_call_enter_delayed ksyn_thread_call_enter_delayed
#define pthread_kern ksyn_pthread_kern

typedef int kern_return_t;
typedef int wait_result_t;
typedef uint32_t mach_port_name_t;
typedef uint32_t mach_msg_type_number_t;

#undef KERN_SUCCESS
#undef KERN_INVALID_ARGUMENT
#undef KERN_NOT_WAITING
#define KERN_SUCCESS          0
#define KERN_INVALID_ARGUMENT 4
#define KERN_NOT_WAITING      48

#define THREAD_AWAKENED    0
#define THREAD_TIMED_OUT   1
#define THREAD_INTERRUPTED 2
#define THREAD_RESTART     3

// struct uthread is the same object as struct ksyn_thread (thread_t)
struct ksyn_thread;
struct uthread;
typedef struct ksyn_thread* thread_t;
#define THREAD_NULL ((thread_t) 0)

struct proc;
typedef struct proc* proc_t;
typedef void* task_t;
typedef void* vm_map_t;

struct turnstile;

typedef void (*thread_continue_t)(void*, wait_result_t);
#define THREAD_CONTINUE_NULL ((thread_continue_t) 0)

typedef enum {
	kThreadWaitNone = 0,
	kThreadWaitPThreadMutex = 0x0d,
	kThreadWaitPThreadRWLockRead = 0x0e,
	kThreadWaitPThreadRWLockWrite = 0x0f,
	kThreadWaitPThreadCondVar = 0x10,
} block_hint_t;

#define THREAD_QOS_UNSPECIFIED 0

// only the process-shared lookup (ksyn_findobj) uses page info, and it never runs here
typedef struct {
	uint64_t object_id;
	uint64_t offset;
} vm_page_info_basic_data_t;
typedef int* vm_page_info_t;
typedef int vm_page_info_flavor_t;
#undef VM_PAGE_INFO_BASIC
#undef VM_PAGE_INFO_BASIC_COUNT
#define VM_PAGE_INFO_BASIC 1
#define VM_PAGE_INFO_BASIC_COUNT ((mach_msg_type_number_t) (sizeof(vm_page_info_basic_data_t) / sizeof(int)))

#undef NSEC_PER_SEC
#define NSEC_PER_SEC 1000000000ull

#define M_PROC 0
#define M_PCB  0
#define PDROP  0x200

// ---- definitions from the kext's kern_internal.h ---------------------------

// "error" flags returned by fail condvar syscalls
#define ECVCLEARED 0x100
#define ECVPREPOST 0x200

struct ksyn_waitq_element {
	TAILQ_ENTRY(ksyn_waitq_element) kwe_list;	/* link to other list members */
	void *          kwe_kwqqueue;            	/* queue blocked on */
	thread_t        kwe_thread;
	uint16_t        kwe_state;			/* state */
	uint16_t        kwe_flags;
	uint32_t        kwe_lockseq;			/* the sequence of the entry */
	uint32_t	kwe_count;			/* upper bound on number of matches still pending */
	uint32_t 	kwe_psynchretval;		/* thread retval */
	void		*kwe_uth;			/* uthread */
};
typedef struct ksyn_waitq_element * ksyn_waitq_element_t;

// the subset of XNU's pthread_callbacks_s (sys/pthread_shims.h) that the synchronizer calls
struct pthread_callbacks_s {
	vm_map_t (*current_map)(void);
	void* (*get_bsdthread_info)(thread_t);
	uint32_t (*get_task_threadmax)(void);
	void* (*proc_get_pthhash)(struct proc*);
	void (*proc_set_pthhash)(struct proc*, void*);
	void (*psynch_wait_cleanup)(void);
	void (*psynch_wait_complete)(uintptr_t kwq, struct turnstile** tstore);
	wait_result_t (*psynch_wait_prepare)(uintptr_t kwq, struct turnstile** tstore, thread_t owner,
			block_hint_t block_hint, uint64_t deadline);
	void (*psynch_wait_update_complete)(struct turnstile* turnstile);
	void (*psynch_wait_update_owner)(uintptr_t kwq, thread_t owner, struct turnstile** tstore);
	kern_return_t (*psynch_wait_wakeup)(uintptr_t kwq, struct ksyn_waitq_element* kwe, struct turnstile** tstore);
	void (*__pthread_testcancel)(int presyscall);
	thread_t (*task_findtid)(task_t, uint64_t);
	void (*thread_deallocate_safe)(thread_t);
	void (*unix_syscall_return)(int) __dead2;
	void* (*uthread_get_uukwe)(struct uthread*);
	int (*uthread_is_cancelled)(struct uthread*);
	void (*uthread_set_returnval)(struct uthread*, int);
	kern_return_t (*vm_map_page_info)(vm_map_t, uint64_t, vm_page_info_flavor_t, vm_page_info_t, mach_msg_type_number_t*);
};
typedef const struct pthread_callbacks_s* pthread_callbacks_t;
extern pthread_callbacks_t pthread_kern;

// ---- locks -----------------------------------------------------------------

// a lock on a futex word: 0 free, 1 locked, 2 locked with waiters
typedef struct {
	uint32_t word;
} ksyn_lock_t;

typedef ksyn_lock_t lck_spin_t;
typedef ksyn_lock_t lck_mtx_t;
typedef void lck_grp_t;
typedef void lck_attr_t;

void ksyn_lock(ksyn_lock_t* lock);
void ksyn_unlock(ksyn_lock_t* lock);

#define pthread_lck_grp  ((lck_grp_t*) 0)
#define pthread_lck_attr ((lck_attr_t*) 0)

#define lck_spin_init(l, grp, attr) ((l)->word = 0)
#define lck_spin_destroy(l, grp)    ((void) (l))
#define lck_spin_lock(l)            ksyn_lock(l)
#define lck_spin_unlock(l)          ksyn_unlock(l)
#define lck_mtx_lock_spin(l)        ksyn_lock(l)
#define lck_mtx_unlock(l)           ksyn_unlock(l)

// sleeps on chan after unlocking mtx (only PDROP is used); woken by wakeup(chan)
int msleep(void* chan, lck_mtx_t* mtx, int pri, const char* wmesg, struct timespec* ts);
void wakeup(void* chan);

// ---- memory ------------------------------------------------------------------

struct ksyn_zone;
typedef struct ksyn_zone* zone_t;

zone_t zinit(size_t size, size_t max, size_t alloc, const char* name);
void* zalloc(zone_t zone);
void zfree(zone_t zone, void* elem);

void* hashinit(int elements, int type, unsigned long* hashmask);
#define FREE(ptr, type) ksyn_free_hash(ptr)
void ksyn_free_hash(void* ptr);

void ksyn_bzero(void* ptr, size_t size);
#undef bzero
#define bzero(p, s) ksyn_bzero((p), (s))

// ---- threads -----------------------------------------------------------------

thread_t current_thread(void);
struct uthread* current_uthread(void);
#undef current_task
#define current_proc() ((proc_t) 0)
#define current_task() ((task_t) 0)
uint64_t thread_tid(thread_t thread);
thread_t port_name_to_thread(mach_port_name_t name);
#define thread_reference(th)  ((void) (th))
#define thread_deallocate(th) ((void) (th))

void thread_block_parameter(thread_continue_t continuation, void* parameter) __dead2;

// ---- time --------------------------------------------------------------------

// "absolute time" is CLOCK_MONOTONIC in nanoseconds
void nanoseconds_to_absolutetime(uint64_t nanoseconds, uint64_t* result);
void clock_absolutetime_interval_to_deadline(uint64_t abstime, uint64_t* result);
void microuptime(struct timeval* tvp);
uint64_t tvtoabstime(struct timeval* tvp);

typedef void* thread_call_t;
void thread_call_enter_delayed(thread_call_t call, uint64_t deadline);

// ---- diagnostics ---------------------------------------------------------------

void ksyn_panic(const char* message) __dead2;
#define panic(fmt, ...) ksyn_panic(fmt)
#define assert(x) ((void) 0)
#define PTHREAD_TRACE(...) do { } while (0)

// ---- the synchronizer's entry points (kern_synch.c) ----------------------------

int _psynch_mutexwait(proc_t p, user_addr_t mutex, uint32_t mgen, uint32_t ugen, uint64_t tid, uint32_t flags, uint32_t* retval);
int _psynch_mutexdrop(proc_t p, user_addr_t mutex, uint32_t mgen, uint32_t ugen, uint64_t tid, uint32_t flags, uint32_t* retval);
int _psynch_cvbroad(proc_t p, user_addr_t cv, uint64_t cvlsgen, uint64_t cvudgen, uint32_t flags, user_addr_t mutex, uint64_t mugen, uint64_t tid, uint32_t* retval);
int _psynch_cvsignal(proc_t p, user_addr_t cv, uint64_t cvlsgen, uint32_t cvugen, int thread_port, user_addr_t mutex, uint64_t mugen, uint64_t tid, uint32_t flags, uint32_t* retval);
int _psynch_cvwait(proc_t p, user_addr_t cv, uint64_t cvlsgen, uint32_t cvugen, user_addr_t mutex, uint64_t mugen, uint32_t flags, int64_t sec, uint32_t nsec, uint32_t* retval);
int _psynch_cvclrprepost(proc_t p, user_addr_t cv, uint32_t cvgen, uint32_t cvugen, uint32_t cvsgen, uint32_t prepocnt, uint32_t preposeq, uint32_t flags, int* retval);
int _psynch_rw_rdlock(proc_t p, user_addr_t rwlock, uint32_t lgenval, uint32_t ugenval, uint32_t rw_wc, int flags, uint32_t* retval);
int _psynch_rw_wrlock(proc_t p, user_addr_t rwlock, uint32_t lgenval, uint32_t ugenval, uint32_t rw_wc, int flags, uint32_t* retval);
int _psynch_rw_unlock(proc_t p, user_addr_t rwlock, uint32_t lgenval, uint32_t ugenval, uint32_t rw_wc, int flags, uint32_t* retval);

void psynch_zoneinit(void);
void _pth_proc_hashinit(proc_t p);
void psynch_wq_cleanup(void* param, void* param1);

extern lck_mtx_t* pthread_list_mlock;
extern thread_call_t psynch_thcall;

// kern_synch.c builds the Darling variant of the code it carries
#define __DARLING__ 1

#endif // KSYN_KERN_H
