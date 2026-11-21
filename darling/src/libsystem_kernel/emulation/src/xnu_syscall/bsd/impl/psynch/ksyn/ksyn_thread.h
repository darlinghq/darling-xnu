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

// Per-thread state of the in-process synchronizer, shared by ksyn_kern.c and
// psynch_inproc.c.

#ifndef KSYN_THREAD_H
#define KSYN_THREAD_H

#include "ksyn_kern.h"

// the state of a frame's wait; the waiter sleeps on this word
enum {
	KSYN_WAIT_IDLE = 0,
	KSYN_WAIT_WAITING,
	KSYN_WAIT_AWAKENED,
	KSYN_WAIT_TIMED_OUT,
	KSYN_WAIT_INTERRUPTED,
	KSYN_WAIT_DONE,             // finished by ksyn_signal_arrived() before a guest handler ran
};

typedef unsigned long long ksyn_sigset_t;

// One psynch call in progress on a thread. A guest signal handler can run in
// the middle of a call (while it sleeps) and make a call of its own, so a
// thread has a chain of frames, one per nesting level. Frames go back to their
// zone, which is never unmapped: other threads may still read a kwe they found
// in a queue.
// The non-local return of a psynch call to its entry: KSYN_SETJMP(buf) gives 0, and 1 again when
// KSYN_LONGJMP(buf) comes back to it. Clang has __builtin_setjmp for the Intel slices but not for
// arm64, which uses ksyn_jmp.S.
#if defined(__arm64__) || defined(__aarch64__)
typedef void* ksyn_jmp_buf[21];
int ksyn_setjmp(ksyn_jmp_buf buf) __attribute__((returns_twice));
void ksyn_longjmp(ksyn_jmp_buf buf) __attribute__((noreturn));
#define KSYN_SETJMP(buf) ksyn_setjmp(buf)
#define KSYN_LONGJMP(buf) ksyn_longjmp(buf)
#else
typedef void* ksyn_jmp_buf[5];
#define KSYN_SETJMP(buf) __builtin_setjmp(buf)
#define KSYN_LONGJMP(buf) __builtin_longjmp(buf, 1)
#endif

struct ksyn_frame {
	struct ksyn_waitq_element kwe;
	uint32_t wait_state;
	uint32_t sleeping;          // in thread_block_parameter() with the caller's signal mask
	bool detached;              // finished early and taken out of the chain (see ksyn_signal_arrived())
	block_hint_t block_hint;
	uint64_t deadline;          // CLOCK_MONOTONIC ns, 0 for none
	ksyn_sigset_t sigmask;      // the caller's signal mask, restored while sleeping
	uint32_t retval;            // the syscall's return value (uthread_set_returnval)
	int error;                  // the syscall's error (unix_syscall_return)
	thread_continue_t continuation; // of the wait in progress
	void* parameter;
	ksyn_jmp_buf return_jmp;    // KSYN_SETJMP buffer of the call's entry
	void** finish_jmp;          // a ksyn_jmp_buf, set while ksyn_signal_arrived() runs the continuation
	struct ksyn_frame* prev;
	struct ksyn_frame* next;
};

// A thread as the synchronizer sees it (thread_t and struct uthread).
struct ksyn_thread {
	struct ksyn_frame* base;    // the frame of the outermost call (allocated on first use)
	struct ksyn_frame* current; // the frame of the innermost call, NULL outside of calls
	struct ksyn_waitq_element idle_kwe; // what uthread_get_uukwe() sees outside of calls
	volatile uint8_t* cancel_slot; // the thread's __PTK_DARLING_CANCEL_MARKED TSD slot
	uint64_t tid;
	mach_port_name_t port;
	bool in_use;
	struct ksyn_thread* registry_next;
};

// the calling thread, created on first use; NULL if memory cannot be allocated
struct ksyn_thread* ksyn_thread_self(void);
// returns the exiting calling thread's ksyn_thread for reuse; does not use TSD
void ksyn_thread_release_exiting(void);

struct ksyn_frame* ksyn_frame_push(struct ksyn_thread* th);
// ends the innermost call (or frees the frame of one that was finished early)
void ksyn_frame_pop(struct ksyn_thread* th, struct ksyn_frame* frame);

// A guest signal handler is about to run on this thread: if the thread sleeps in
// a psynch wait, end that wait first, as XNU does before it delivers a signal.
void ksyn_signal_arrived(struct ksyn_thread* th);

void ksyn_block_signals(ksyn_sigset_t* old);
void ksyn_restore_signals(const ksyn_sigset_t* old);

// one-time setup; false if memory cannot be allocated
bool ksyn_init(void);
// runs the deferred kwq cleanup if it is due
void ksyn_run_due_cleanup(void);

void ksyn_prefork(void);
void ksyn_postfork_parent(void);
void ksyn_postfork_child(void);

// in kern_synch.c: forgets the kwqs waiting to be freed (the child of a fork)
void ksyn_postfork_child_reset(void);

#endif // KSYN_THREAD_H
