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

// psynch calls on process-private pthread objects, served inside the process by
// the pthread kext's synchronizer (ksyn/kern_synch.c) instead of by darlingserver.
//
// XNU keeps the wait queues of process-shared objects in a global table and all
// others in a per-process table, choosing by the flags of the call. The calls
// that XNU would serve from the per-process table are the ones handled here; no
// wait queue is ever reachable from both places.

#include <darling/emulation/xnu_syscall/bsd/impl/psynch/psynch_inproc.h>

#include <pthread/tsd_private.h>
#include <darling/emulation/common/tsd.h>

#include <darling/emulation/xnu_syscall/bsd/impl/bsdthread/bsdthread_register.h>

// last: see ksyn/ksyn_kern.h
#include "ksyn/ksyn_thread.h"

// the pshared bits of the flags, as the kext tests them (PTHREAD_PSHARED_FLAGS_MASK, PTHREAD_PROCESS_SHARED)
#define PSYNCH_PSHARED_FLAGS_MASK 0x30
#define PSYNCH_PROCESS_SHARED     0x10

enum {
	PSYNCH_MODE_INPROC = 0,
	PSYNCH_MODE_SERVER,
};

static int psynch_mode;

// DARLING_PSYNCH_SERVER=1 in the environment of a program sends every psynch
// call to darlingserver, as before in-process synchronization existed (for
// comparisons and debugging). Read once, at libsystem_kernel's initialization.
void psynch_inproc_init(const char* envp[])
{
	static const char setting[] = "DARLING_PSYNCH_SERVER=1";

	for (size_t i = 0; envp != NULL && envp[i] != NULL; i++)
	{
		size_t k = 0;
		while (setting[k] != '\0' && envp[i][k] == setting[k])
			k++;
		if (setting[k] == '\0' && envp[i][k] == '\0')
			psynch_mode = PSYNCH_MODE_SERVER;
	}
}

bool psynch_inproc_handles(uint32_t flags)
{
#ifdef VARIANT_DYLD
	(void) flags;
	return false;
#else
	if ((flags & PSYNCH_PSHARED_FLAGS_MASK) == PSYNCH_PROCESS_SHARED)
		return false;

	// before libpthread has registered there is no TSD (and no second thread to contend with)
	if (!bsdthread_registered())
		return false;

	return psynch_mode == PSYNCH_MODE_INPROC;
#endif
}

// Starts a call: blocks signals (so that a handler of this thread cannot re-enter
// the synchronizer while it holds a lock; they are let through while the call
// sleeps) and pushes a frame for the call.
static struct ksyn_frame* psynch_enter(void)
{
	ksyn_sigset_t sigmask;
	struct ksyn_thread* th;
	struct ksyn_frame* frame;

	ksyn_block_signals(&sigmask);

	if (!ksyn_init())
		ksyn_panic("cannot allocate the wait queue table");

	th = ksyn_thread_self();
	// (libpthread sets it at thread start and updates it in the child of a fork)
	th->port = (mach_port_name_t) (uintptr_t) _pthread_getspecific_direct(_PTHREAD_TSD_SLOT_MACH_THREAD_SELF);

	frame = ksyn_frame_push(th);
	frame->sigmask = sigmask;

	ksyn_run_due_cleanup();
	return frame;
}

// Ends a call: the kext's return value, or -errno (with ECVCLEARED/ECVPREPOST, as XNU returns them)
static long psynch_leave(struct ksyn_frame* frame)
{
	ksyn_sigset_t sigmask = frame->sigmask;
	int error = frame->error;
	uint32_t retval = frame->retval;

	ksyn_frame_pop(current_thread(), frame);
	ksyn_restore_signals(&sigmask);

	if (error != 0)
		return -error;
	return retval;
}

// A call that blocks ends in its continuation, which returns to the
// KSYN_SETJMP below through unix_syscall_return().
#define PSYNCH_CALL(expr) \
	do { \
		struct ksyn_frame* volatile frame = psynch_enter(); \
		if (KSYN_SETJMP(frame->return_jmp) == 0) \
			frame->error = (expr); \
		return psynch_leave(frame); \
	} while (0)

#define RETVAL (&frame->retval)
#define UADDR(p) ((user_addr_t) (uintptr_t) (p))

long psynch_inproc_mutexwait(void* mutex, uint32_t mgen, uint32_t ugen, uint64_t tid, uint32_t flags)
{
	PSYNCH_CALL(_psynch_mutexwait(NULL, UADDR(mutex), mgen, ugen, tid, flags, RETVAL));
}

long psynch_inproc_mutexdrop(void* mutex, uint32_t mgen, uint32_t ugen, uint64_t tid, uint32_t flags)
{
	PSYNCH_CALL(_psynch_mutexdrop(NULL, UADDR(mutex), mgen, ugen, tid, flags, RETVAL));
}

long psynch_inproc_cvwait(void* cv, uint64_t cvlsgen, uint32_t cvugen, void* mutex, uint64_t mugen,
		uint32_t flags, int64_t sec, uint32_t nsec)
{
	PSYNCH_CALL(_psynch_cvwait(NULL, UADDR(cv), cvlsgen, cvugen, UADDR(mutex), mugen, flags, sec, nsec, RETVAL));
}

long psynch_inproc_cvsignal(void* cv, uint64_t cvlsgen, uint32_t cvugen, int32_t thread_port, void* mutex,
		uint64_t mugen, uint64_t tid, uint32_t flags)
{
	PSYNCH_CALL(_psynch_cvsignal(NULL, UADDR(cv), cvlsgen, cvugen, thread_port, UADDR(mutex), mugen, tid, flags, RETVAL));
}

long psynch_inproc_cvbroad(void* cv, uint64_t cvlsgen, uint64_t cvudgen, uint32_t flags, void* mutex,
		uint64_t mugen, uint64_t tid)
{
	PSYNCH_CALL(_psynch_cvbroad(NULL, UADDR(cv), cvlsgen, cvudgen, flags, UADDR(mutex), mugen, tid, RETVAL));
}

long psynch_inproc_cvclrprepost(void* cv, uint32_t cvgen, uint32_t cvugen, uint32_t cvsgen, uint32_t prepocnt,
		uint32_t preposeq, uint32_t flags)
{
	PSYNCH_CALL(_psynch_cvclrprepost(NULL, UADDR(cv), cvgen, cvugen, cvsgen, prepocnt, preposeq, flags, (int*) RETVAL));
}

long psynch_inproc_rw_rdlock(void* rwlock, uint32_t lgenval, uint32_t ugenval, uint32_t rw_wc, int flags)
{
	PSYNCH_CALL(_psynch_rw_rdlock(NULL, UADDR(rwlock), lgenval, ugenval, rw_wc, flags, RETVAL));
}

long psynch_inproc_rw_wrlock(void* rwlock, uint32_t lgenval, uint32_t ugenval, uint32_t rw_wc, int flags)
{
	PSYNCH_CALL(_psynch_rw_wrlock(NULL, UADDR(rwlock), lgenval, ugenval, rw_wc, flags, RETVAL));
}

long psynch_inproc_rw_unlock(void* rwlock, uint32_t lgenval, uint32_t ugenval, uint32_t rw_wc, int flags)
{
	PSYNCH_CALL(_psynch_rw_unlock(NULL, UADDR(rwlock), lgenval, ugenval, rw_wc, flags, RETVAL));
}

void psynch_inproc_signal_arrived(void)
{
#ifndef VARIANT_DYLD
	if (psynch_mode != PSYNCH_MODE_INPROC || !bsdthread_registered())
		return;

	struct ksyn_thread* th = (struct ksyn_thread*) _pthread_getspecific_direct(__PTK_DARLING_PSYNCH);
	if (th != NULL)
		ksyn_signal_arrived(th);
#endif
}

void psynch_inproc_thread_exit(void)
{
#ifndef VARIANT_DYLD
	ksyn_sigset_t sigmask;

	if (psynch_mode != PSYNCH_MODE_INPROC)
		return;

	// Signals stay blocked: the thread runs no more guest code, and a handler
	// must not find this thread's state through its stale TSD once another
	// thread reuses it. The rest of thread termination blocks them anyway.
	ksyn_block_signals(&sigmask);
	ksyn_thread_release_exiting();
#endif
}

void psynch_inproc_prefork(unsigned long long* sigmask)
{
	ksyn_block_signals(sigmask);
	ksyn_prefork();
}

void psynch_inproc_postfork_parent(const unsigned long long* sigmask)
{
	ksyn_postfork_parent();
	ksyn_restore_signals(sigmask);
}

void psynch_inproc_postfork_child(const unsigned long long* sigmask)
{
	ksyn_postfork_child();
	ksyn_restore_signals(sigmask);
}
