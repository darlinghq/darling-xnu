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

// The task's side of shared message queues (darlingserver/mach-sq.h, darlingserver's duct-tape/src/mach_sq.c).
//
// A send is done here when it is a simple message (no descriptors, no reply port, no voucher) of at most
// DSQ_MSG_MAX bytes with a COPY_SEND or MAKE_SEND destination right (on every architecture) to a port of this task that the server
// put in shared-queue mode. The message goes into a node of the port's queue; the receiver gets it from
// there. A receive is done here when the port is in the mode and its next message is such a node. Whatever
// else the queue holds (messages the server delivered) is received through the server, in queue order.
//
// Everything else, and anything that fails a check here, goes to the server unchanged, which answers as it
// always did: this code never produces an error the server would not.

#include <darling/emulation/xnu_syscall/mach/impl/mach_sq.h>

#include <mach/mach_traps.h>
#include <mach/message.h>
#include <mach/port.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/common/simple.h>
#include <darling/emulation/common/tsd.h>
#include <darling/emulation/conversion/duct_errno.h>
#include <darling/emulation/conversion/signal/sigaction.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/xnu_syscall/bsd/impl/bsdthread/bsdthread_register.h>

#include <darlingserver/rpc.h>
#include <darlingserver/rpc-supplement.h>
#include <darlingserver/mach-sq.h>

#ifdef VARIANT_DYLD

// dyld sends everything to the server

void mach_sq_init(const char* envp[])
{
	(void) envp;
}

void mach_sq_prepare(void)
{
}

bool mach_sq_msg(mach_msg_header_t* msg, mach_msg_option_t option, mach_msg_size_t send_size,
		mach_msg_size_t rcv_size, mach_port_name_t rcv_name, mach_msg_timeout_t timeout,
		mach_msg_header_t* rcv_msg, mach_msg_return_t* result)
{
	return false;
}

bool mach_sq_defer_signal(int linux_signum, const struct linux_siginfo* info)
{
	return false;
}

void mach_sq_postfork_child(void)
{
}

#else // !VARIANT_DYLD

#define SQ_FUTEX_WAIT_BITSET      9
#define SQ_FUTEX_WAKE             1
#define SQ_FUTEX_BITSET_MATCH_ANY 0xffffffff
#define SQ_CLOCK_MONOTONIC        1
#define SQ_PROT_READ              1
#define SQ_PROT_WRITE             2
#define SQ_MAP_SHARED             1
#define SQ_MADV_DONTFORK          10

// CAS attempts on a contended queue word before the call goes to the server
#define SQ_ATTEMPTS 64

// the option bits darlingserver's mach_msg_overwrite_trap() keeps (XNU's MACH_MSG_OPTION_USER, which the
// SDK only defines for the kernel); it ignores all others
#define SQ_OPTION_USER (MACH_SEND_MSG | MACH_SEND_TIMEOUT | MACH_SEND_NOTIFY | MACH_SEND_OVERRIDE | \
		MACH_SEND_TRAILER | MACH_SEND_NOIMPORTANCE | MACH_SEND_SYNC_OVERRIDE | MACH_SEND_PROPAGATE_QOS | \
		MACH_SEND_SYNC_BOOTSTRAP_CHECKIN | MACH_MSG_STRICT_REPLY | MACH_RCV_GUARDED_DESC | \
		MACH_RCV_MSG | MACH_RCV_TIMEOUT | MACH_RCV_LARGE | MACH_RCV_LARGE_IDENTITY | MACH_RCV_VOUCHER | \
		MACH_RCV_TRAILER_MASK | MACH_RCV_SYNC_WAIT | MACH_RCV_SYNC_PEEK)

// the option bits a shortcut covers (after the server's own MACH_MSG_OPTION_USER mask)
#define SQ_SEND_OPTIONS (MACH_SEND_MSG | MACH_SEND_TIMEOUT | MACH_SEND_NOIMPORTANCE)
#define SQ_RCV_OPTIONS  (MACH_RCV_MSG | MACH_RCV_TIMEOUT | MACH_RCV_LARGE | MACH_RCV_LARGE_IDENTITY | \
		MACH_RCV_VOUCHER | MACH_RCV_GUARDED_DESC | MACH_RCV_TRAILER_MASK)

enum
{
	SQ_STATE_NONE,      // not attached (yet)
	SQ_STATE_ATTACHING, // a thread is attaching
	SQ_STATE_READY,     // `sq_view` is usable
	SQ_STATE_OFF,       // kill switch, or attaching failed
};

static int sq_state;
static dsq_view_t sq_view;
static int sq_pid;

struct sq_timespec
{
	long tv_sec;
	long tv_nsec;
};

struct sq_iovec
{
	void* base;
	unsigned long len;
};

// ---- kill switch and region ------------------------------------------------------

// DARLING_MACH_SERVER=1 in the environment of a program sends every Mach message to darlingserver, as
// before shared queues existed (for comparisons and debugging). Read once, at libsystem_kernel's
// initialization; the same parser as DARLING_PSYNCH_SERVER's.
void mach_sq_init(const char* envp[])
{
	static const char setting[] = "DARLING_MACH_SERVER=1";

	for (unsigned long i = 0; envp != NULL && envp[i] != NULL; i++)
	{
		unsigned long k = 0;
		while (setting[k] != '\0' && envp[i][k] == setting[k])
			k++;
		if (setting[k] == '\0' && envp[i][k] == '\0')
			sq_state = SQ_STATE_OFF;
	}
}

static inline bool sq_ready(void)
{
	return __atomic_load_n(&sq_state, __ATOMIC_ACQUIRE) == SQ_STATE_READY &&
		(__atomic_load_n(&sq_view.s->hdr.flags, __ATOMIC_ACQUIRE) & DSQ_FLAG_POISONED) == 0;
}

static void* sq_map(int fd, unsigned long size, int prot)
{
	long result;

#ifdef __NR_mmap2
	result = LINUX_SYSCALL(__NR_mmap2, NULL, size, prot, SQ_MAP_SHARED, fd, 0);
#else
	result = LINUX_SYSCALL(__NR_mmap, NULL, size, prot, SQ_MAP_SHARED, fd, 0);
#endif
	if (result < 0 && result > -4096)
		return NULL;

	// a forked child must not see (or keep) the parent's queues
	LINUX_SYSCALL(__NR_madvise, result, size, SQ_MADV_DONTFORK);
	return (void*) result;
}

static void sq_attach(void)
{
	int s_fd = -1, q_fd = -1;
	void* s_map = NULL;
	void* q_map = NULL;
	int code;

	code = dserver_rpc_mach_sq_attach(&s_fd, &q_fd);
	if (code == 0)
	{
		s_map = sq_map(s_fd, DSQ_S_SIZE, SQ_PROT_READ);
		q_map = sq_map(q_fd, DSQ_Q_SIZE, SQ_PROT_READ | SQ_PROT_WRITE);
	}
	if (s_fd >= 0)
		LINUX_SYSCALL(__NR_close, s_fd);
	if (q_fd >= 0)
		LINUX_SYSCALL(__NR_close, q_fd);

	if (s_map == NULL || q_map == NULL)
	{
		// the server may already put ports in the mode; with the state off, they are only used through it
		__atomic_store_n(&sq_state, SQ_STATE_OFF, __ATOMIC_RELEASE);
		return;
	}

	const dsq_s_header_t* hdr = &((const dsq_s_region_t*) s_map)->hdr;
	if (hdr->magic != DSQ_MAGIC || hdr->version != DSQ_VERSION || hdr->s_size != DSQ_S_SIZE ||
			hdr->q_size != DSQ_Q_SIZE)
	{
		__atomic_store_n(&sq_state, SQ_STATE_OFF, __ATOMIC_RELEASE);
		return;
	}

	sq_pid = LINUX_SYSCALL0(__NR_getpid);
	sq_view.s = s_map;
	sq_view.q = q_map;
	__atomic_store_n(&sq_state, SQ_STATE_READY, __ATOMIC_RELEASE);
}

void mach_sq_prepare(void)
{
	int expected = SQ_STATE_NONE;

	// before libpthread has registered there is no TSD (needed for signal deferral)
	if (!bsdthread_registered())
		return;

	if (__atomic_load_n(&sq_state, __ATOMIC_ACQUIRE) != SQ_STATE_NONE)
		return;
	if (!__atomic_compare_exchange_n(&sq_state, &expected, SQ_STATE_ATTACHING, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE))
		return;

	sq_attach();
}

void mach_sq_postfork_child(void)
{
	// the mappings were not inherited (MADV_DONTFORK), and the child's ports are a new space in the server
	sq_view.s = NULL;
	sq_view.q = NULL;
	sq_pid = 0;
	// a child of a program that has the kill switch set (or could not attach) keeps it
	if (__atomic_load_n(&sq_state, __ATOMIC_RELAXED) != SQ_STATE_OFF)
		__atomic_store_n(&sq_state, SQ_STATE_NONE, __ATOMIC_RELEASE);
}

// ---- signals -----------------------------------------------------------------------

// Between making a message visible (or taking one) and waking the thread that waits for that, this
// thread must not stop in a signal handler that blocks: Darling's thread suspension and the delivery of
// a signal as a Mach exception both do. Such signals are kept and raised again after the wake.

#define SQ_DEFER_MAX 4

struct sq_defer
{
	int count;
	int signum[SQ_DEFER_MAX];
	unsigned char info[SQ_DEFER_MAX][128];
};

static inline void sq_defer_begin(struct sq_defer* defer)
{
	defer->count = 0;
	_pthread_setspecific_direct(__PTK_DARLING_MACH_SQ, defer);
	__atomic_signal_fence(__ATOMIC_SEQ_CST);
}

static void sq_defer_end(struct sq_defer* defer)
{
	__atomic_signal_fence(__ATOMIC_SEQ_CST);
	_pthread_setspecific_direct(__PTK_DARLING_MACH_SQ, NULL);
	__atomic_signal_fence(__ATOMIC_SEQ_CST);

	if (defer->count == 0)
		return;

	int tid = LINUX_SYSCALL0(__NR_gettid);
	for (int i = 0; i < defer->count; i++)
		LINUX_SYSCALL(__NR_rt_tgsigqueueinfo, sq_pid, tid, defer->signum[i], defer->info[i]);
}

bool mach_sq_defer_signal(int linux_signum, const struct linux_siginfo* info)
{
	if (!bsdthread_registered())
		return false;

	struct sq_defer* defer = _pthread_getspecific_direct(__PTK_DARLING_MACH_SQ);
	if (defer == NULL || defer->count >= SQ_DEFER_MAX || info == NULL)
		return false;

	defer->signum[defer->count] = linux_signum;
	__builtin_memcpy(defer->info[defer->count], info, sizeof(defer->info[0]));
	defer->count++;
	return true;
}

// ---- helpers -----------------------------------------------------------------------

// the fault-checked copies: a bad user buffer gives an error, as it does in the server
static bool sq_copyin(void* dst, const void* src, unsigned long size)
{
	struct sq_iovec local = { dst, size };
	struct sq_iovec remote = { (void*) src, size };
	long ret = LINUX_SYSCALL(__NR_process_vm_readv, sq_pid, &local, 1, &remote, 1, 0);
	return ret == (long) size;
}

static bool sq_copyout(void* dst, const void* src, unsigned long size)
{
	struct sq_iovec local = { (void*) src, size };
	struct sq_iovec remote = { dst, size };
	long ret = LINUX_SYSCALL(__NR_process_vm_writev, sq_pid, &local, 1, &remote, 1, 0);
	return ret == (long) size;
}

static void sq_futex_wake(uint64_t* word, int count)
{
	// the low half of the queue word; shared with the server, so not a private futex
	LINUX_SYSCALL(__NR_futex, (uint32_t*) word, SQ_FUTEX_WAKE, count, NULL, NULL, 0);
}

static void sq_now(struct sq_timespec* ts)
{
	LINUX_SYSCALL(__NR_clock_gettime, SQ_CLOCK_MONOTONIC, ts);
}

// `timeout` ms from now; no timeout is a deadline that never comes
static void sq_deadline(bool timed, mach_msg_timeout_t timeout, struct sq_timespec* deadline)
{
	if (!timed)
	{
		deadline->tv_sec = (long) (~0ul >> 2);
		deadline->tv_nsec = 0;
		return;
	}
	sq_now(deadline);
	deadline->tv_sec += timeout / 1000;
	deadline->tv_nsec += (long) (timeout % 1000) * 1000000;
	if (deadline->tv_nsec >= 1000000000)
	{
		deadline->tv_sec++;
		deadline->tv_nsec -= 1000000000;
	}
}

// the ms left until `deadline` (rounded up), for a call that goes to the server after a local wait
static mach_msg_timeout_t sq_remaining(const struct sq_timespec* deadline)
{
	struct sq_timespec now;
	long long ns;

	sq_now(&now);
	ns = (long long) (deadline->tv_sec - now.tv_sec) * 1000000000ll + (deadline->tv_nsec - now.tv_nsec);
	if (ns <= 0)
		return 0;
	ns = (ns + 999999) / 1000000;
	return (ns > 0xffffffffll) ? 0xffffffffu : (mach_msg_timeout_t) ns;
}

// waits until the low half of `word` is no longer `expected`. the absolute timeout also makes a signal
// handler end the wait with EINTR, even one installed with SA_RESTART
static long sq_futex_wait(uint64_t* word, uint32_t expected, const struct sq_timespec* deadline)
{
	return LINUX_SYSCALL(__NR_futex, (uint32_t*) word, SQ_FUTEX_WAIT_BITSET, expected, deadline, NULL,
		SQ_FUTEX_BITSET_MATCH_ANY);
}

// a receive-only call through the server, the same as the generic trap does it
static mach_msg_return_t sq_server_receive(void* rcv_addr, mach_msg_option_t option, mach_msg_size_t rcv_size,
		mach_port_name_t rcv_name, mach_msg_timeout_t timeout, bool may_wait_locally)
{
	int code;

	for (;;)
	{
		if (may_wait_locally)
			code = dserver_rpc_mach_msg_sq_receive(rcv_addr, option & ~MACH_SEND_MSG, rcv_size, rcv_name, timeout);
		else
			code = dserver_rpc_mach_msg_overwrite(rcv_addr, option & ~MACH_SEND_MSG, 0, rcv_size, rcv_name,
				timeout, MACH_PORT_NULL, NULL);

		if (code == -DSERVER_RPC_RESTART)
			continue;
		if (code == -LINUX_EINTR)
		{
			if (option & MACH_RCV_INTERRUPT)
				return MACH_RCV_INTERRUPTED;
			continue;
		}
		if (code < 0)
		{
			__simple_printf("mach_msg_overwrite failed (internally): %d\n", code);
			__simple_abort();
		}
		return code;
	}
}

// ---- send ---------------------------------------------------------------------------

enum
{
	SQ_SEND_DONE,   // sent here
	SQ_SEND_SERVER, // not a shortcut: the send goes to the server as it is
	SQ_SEND_FULL,   // the queue was full until the deadline (or a signal): the server finishes the send
};

static int sq_send(const mach_msg_header_t* msg, mach_msg_option_t option, mach_msg_size_t send_size,
		mach_msg_timeout_t timeout, bool* interrupted)
{
	uint8_t buffer[DSQ_MSG_MAX];
	mach_msg_header_t header;
	dsq_name_entry_t entry;
	mach_msg_type_name_t remote;
	bool make_send;
	uint32_t index;
	int result;

	if ((option & MACH_SEND_MSG) == 0 || (option & ~(SQ_SEND_OPTIONS | SQ_RCV_OPTIONS)) != 0)
		return SQ_SEND_SERVER;
	if (send_size < DSQ_MSG_MIN || send_size > DSQ_MSG_MAX || (send_size & 3) != 0)
		return SQ_SEND_SERVER;

	// one copy: what is checked is what is queued
	if (!sq_copyin(buffer, msg, send_size))
		return SQ_SEND_SERVER;
	__builtin_memcpy(&header, buffer, sizeof(header));

	// a simple message with only a destination: no descriptors, no reply port, no voucher
	if ((header.msgh_bits & ~MACH_MSGH_BITS_USER) != 0 ||
			(header.msgh_bits & (MACH_MSGH_BITS_COMPLEX | MACH_MSGH_BITS_LOCAL_MASK)) != 0 ||
			header.msgh_local_port != MACH_PORT_NULL ||
			(MACH_MSGH_BITS_VOUCHER(header.msgh_bits) != 0 && header.msgh_voucher_port != MACH_PORT_NULL))
		return SQ_SEND_SERVER;

	remote = MACH_MSGH_BITS_REMOTE(header.msgh_bits);
	if (remote != MACH_MSG_TYPE_COPY_SEND && remote != MACH_MSG_TYPE_MAKE_SEND)
		return SQ_SEND_SERVER;
	make_send = remote == MACH_MSG_TYPE_MAKE_SEND;

	if (!dsq_name_lookup(&sq_view, header.msgh_remote_port, &entry))
		return SQ_SEND_SERVER;
	if ((entry.rights & (make_send ? DSQ_RIGHT_RECEIVE : DSQ_RIGHT_SEND)) == 0)
		return SQ_SEND_SERVER;
	// a port set has no queue, and a mk_timer port is not a destination for the task's messages
	if ((entry.rights & (DSQ_RIGHT_SET | DSQ_RIGHT_NOSEND)) != 0)
		return SQ_SEND_SERVER;

	index = dsq_task_node_alloc(&sq_view);
	if (index == DSQ_NONE)
		return SQ_SEND_SERVER;

	// the message as the server sees it: only the destination disposition, and the size that was sent
	dsq_node_t* node = dsq_node(&sq_view, index);
	header.msgh_bits = MACH_MSGH_BITS(remote, 0);
	header.msgh_size = send_size;
	header.msgh_voucher_port = MACH_PORT_NULL;
	__builtin_memcpy(buffer, &header, sizeof(header));
	__builtin_memcpy(node->msg, buffer, send_size);
	node->flags = DSQ_NODE_INLINE | (make_send ? DSQ_NODE_MADE_SEND : 0);
	node->size = (uint16_t) send_size;
	node->aux = dsq_tokens_current(&sq_view);

	dsq_q_slot_t* qslot = &sq_view.q->slots[entry.slot];
	const dsq_s_slot_t* sslot = &sq_view.s->slots[entry.slot];
	struct sq_timespec deadline;
	bool have_deadline = false;

	for (;;)
	{
		struct sq_defer defer;
		uint32_t qlimit = __atomic_load_n(&sslot->qlimit, __ATOMIC_ACQUIRE);

		sq_defer_begin(&defer);
		uint32_t set;
		result = dsq_publish(&sq_view, entry.slot, index, entry.epoch, qlimit, SQ_ATTEMPTS, NULL, &set);
		if (result == DSQ_OK)
		{
			if (__atomic_load_n(&qslot->rcv_waiters, __ATOMIC_ACQUIRE) != 0)
				sq_futex_wake(&qslot->head, 1);
			// a port in a set: the set's receivers wait on the set's word
			if (set != DSQ_NONE && __atomic_load_n(&sq_view.q->slots[set].rcv_waiters, __ATOMIC_ACQUIRE) != 0)
				sq_futex_wake((uint64_t*) &sq_view.q->slots[set].setword, 1);
			sq_defer_end(&defer);
			return SQ_SEND_DONE;
		}
		sq_defer_end(&defer);

		if (result != DSQ_FULL)
			break;

		// a full queue: wait for room, then the server finishes the send (3.1 of the design: a send that
		// fails returns its rights to the sender, which only the server does)
		if ((option & MACH_SEND_TIMEOUT) && timeout == 0)
			break;
		if (!have_deadline)
		{
			sq_deadline((option & MACH_SEND_TIMEOUT) != 0, timeout, &deadline);
			have_deadline = true;
		}

		__atomic_add_fetch(&qslot->snd_waiters, 1, __ATOMIC_ACQ_REL);
		uint64_t d = dsq_load(&qslot->deq);
		uint64_t h = dsq_load(&qslot->head);
		long ret = 0;
		if (!DSQ_IS_CLOSED(h) && DSQ_LEN(h, d) >= __atomic_load_n(&sslot->qlimit, __ATOMIC_ACQUIRE))
			ret = sq_futex_wait(&qslot->deq, (uint32_t) d, &deadline);
		__atomic_sub_fetch(&qslot->snd_waiters, 1, __ATOMIC_ACQ_REL);

		if (ret == -LINUX_ETIMEDOUT)
		{
			dsq_task_node_free(&sq_view, index);
			return SQ_SEND_FULL;
		}
		if (ret == -LINUX_EINTR)
		{
			*interrupted = true;
			dsq_task_node_free(&sq_view, index);
			return SQ_SEND_FULL;
		}
		// room (or a close, or a qlimit change): try again
	}

	dsq_task_node_free(&sq_view, index);
	return SQ_SEND_SERVER;
}

// ---- receive --------------------------------------------------------------------------

// A publish wakes one waiting receiver. One that returns without taking the message (MACH_RCV_TOO_LARGE with
// MACH_RCV_LARGE leaves it queued) passes the wake on, so that another waiting receiver gets it, as XNU's
// ipc_mqueue_post() goes on to the next receiver after giving one the too-large indication.
static void sq_pass_wake(uint32_t* waiters, uint64_t* word)
{
	if (__atomic_load_n(waiters, __ATOMIC_ACQUIRE) != 0)
		sq_futex_wake(word, 1);
}

// The slot a receive uses is not the one its name table entry described any more: a right of the name
// changed (the receive goes on with the entry as it is now), or the slot was closed and given to another
// port (the receive is for the port that was). Only the first is for the receive to continue.
static bool sq_receive_refresh(mach_port_name_t rcv_name, dsq_name_entry_t* entry, uint32_t gen)
{
	dsq_name_entry_t now;

	if (!dsq_name_lookup(&sq_view, rcv_name, &now) || (now.rights & DSQ_RIGHT_RECEIVE) == 0 || now.slot != entry->slot)
		return false;
	if (__atomic_load_n(&sq_view.s->slots[now.slot].gen, __ATOMIC_ACQUIRE) != gen)
		return false;
	*entry = now;
	return true;
}

static bool sq_set_receive(dsq_name_entry_t entry, void* rcv_addr, mach_msg_option_t option, mach_msg_size_t rcv_size,
		mach_port_name_t rcv_name, mach_msg_timeout_t timeout, mach_msg_return_t* result);

static bool sq_receive(void* rcv_addr, mach_msg_option_t option, mach_msg_size_t rcv_size,
		mach_port_name_t rcv_name, mach_msg_timeout_t timeout, mach_msg_return_t* result)
{
	dsq_name_entry_t entry;
	struct sq_timespec deadline;
	bool have_deadline = false;
	uint32_t trailer_size = dsq_trailer_size(option);

	if ((option & ~(SQ_RCV_OPTIONS | MACH_SEND_MSG | SQ_SEND_OPTIONS)) != 0 || rcv_addr == NULL)
		return false;
	if (!dsq_name_lookup(&sq_view, rcv_name, &entry) || (entry.rights & DSQ_RIGHT_RECEIVE) == 0)
		return false;
	if (entry.rights & DSQ_RIGHT_SET)
		return sq_set_receive(entry, rcv_addr, option, rcv_size, rcv_name, timeout, result);

	dsq_q_slot_t* qslot = &sq_view.q->slots[entry.slot];
	const dsq_s_slot_t* sslot = &sq_view.s->slots[entry.slot];
	// the port this call is for; the slot's number goes to another port whenever this one leaves the mode
	const uint32_t gen = __atomic_load_n(&sslot->gen, __ATOMIC_ACQUIRE);
	unsigned int stale = 0;

	for (unsigned int round = 0;; round++)
	{
		uint64_t d;
		uint32_t index;
		int r;

		if (round > 1000000)
			return false;
		r = dsq_head(&sq_view, entry.slot, entry.epoch, &d, &index);
		if (r == DSQ_RETRY)
			continue;
		if (r == DSQ_STALE)
		{
			if (sq_receive_refresh(rcv_name, &entry, gen) && ++stale < 100000)
				continue;
			// the port is gone from the slot (or the epoch change is not finished): what the server says
			// about the name now is the answer
			*result = sq_server_receive(rcv_addr, option, rcv_size, rcv_name,
				(option & MACH_RCV_TIMEOUT) && have_deadline ? sq_remaining(&deadline) : timeout, false);
			return true;
		}
		if (r == DSQ_CLOSED_Q || r == DSQ_CORRUPT || !sq_ready())
			return false;

		if (r == DSQ_EMPTY)
		{
			long ret = 0;
			uint64_t h;

			if ((option & MACH_RCV_TIMEOUT) && timeout == 0)
			{
				*result = MACH_RCV_TIMED_OUT;
				return true;
			}
			if (!have_deadline)
			{
				sq_deadline((option & MACH_RCV_TIMEOUT) != 0, timeout, &deadline);
				have_deadline = true;
			}

			__atomic_add_fetch(&qslot->rcv_waiters, 1, __ATOMIC_ACQ_REL);
			// deq before head, as everywhere: an open head means `d` is the queue's
			d = dsq_load(&qslot->deq);
			h = dsq_load(&qslot->head);
			// the wait is only for the queue of this call's port: a head with another epoch is looked at
			// from the top
			if (!DSQ_IS_CLOSED(h) && DSQ_H_EPOCH(h) == entry.epoch && DSQ_LEN(h, d) == 0)
				ret = sq_futex_wait(&qslot->head, (uint32_t) h, &deadline);
			__atomic_sub_fetch(&qslot->rcv_waiters, 1, __ATOMIC_ACQ_REL);

			h = dsq_load(&qslot->head);
			if (DSQ_IS_CLOSED(h))
			{
				uint32_t reason = __atomic_load_n(&sslot->close_reason, __ATOMIC_ACQUIRE);
				// what XNU's blocked receivers get when the receive right moves or is destroyed, if the close
				// was this call's port's: the slot may have been given to another port since, and closed again
				// with that port's reason (the server changes gen before it writes a new port's reason)
				if ((reason == DSQ_CLOSE_MOVED || reason == DSQ_CLOSE_DESTROYED) &&
						__atomic_load_n(&sslot->gen, __ATOMIC_ACQUIRE) == gen)
				{
					*result = MACH_RCV_PORT_CHANGED;
					return true;
				}
				*result = sq_server_receive(rcv_addr, option, rcv_size, rcv_name,
					(option & MACH_RCV_TIMEOUT) ? sq_remaining(&deadline) : timeout, false);
				return true;
			}
			if (ret == -LINUX_ETIMEDOUT && DSQ_LEN(h, dsq_load(&qslot->deq)) == 0)
			{
				*result = MACH_RCV_TIMED_OUT;
				return true;
			}
			if (ret == -LINUX_EINTR && (option & MACH_RCV_INTERRUPT))
			{
				*result = MACH_RCV_INTERRUPTED;
				return true;
			}
			continue;
		}

		dsq_node_t* node = dsq_node(&sq_view, index);
		uint32_t flags = __atomic_load_n(&node->flags, __ATOMIC_RELAXED);
		uint32_t size = __atomic_load_n(&node->size, __ATOMIC_RELAXED);
		bool inline_node = index < DSQ_TNODES && (flags & DSQ_NODE_INLINE) != 0 &&
			size >= DSQ_MSG_MIN && size <= DSQ_MSG_MAX;

		// the fields are valid only if the node was still the head after they were read
		if (dsq_load(&qslot->deq) != d)
			continue;

		if (inline_node && size + trailer_size > rcv_size)
		{
			if ((option & MACH_RCV_LARGE) == 0)
				inline_node = false; // the server destroys it, as XNU does
			else
			{
				// left queued; the caller learns the size it needs (as mach_msg_receive_results())
				mach_msg_return_t mr = MACH_RCV_TOO_LARGE;
				if (rcv_size >= 16)
				{
					if ((option & MACH_RCV_LARGE_IDENTITY) &&
							!sq_copyout((char*) rcv_addr + 12, &rcv_name, sizeof(rcv_name)))
						mr = MACH_RCV_INVALID_DATA;
					if (!sq_copyout((char*) rcv_addr + 4, &size, sizeof(size)))
						mr = MACH_RCV_INVALID_DATA;
				}
				sq_pass_wake(&qslot->rcv_waiters, &qslot->head);
				*result = mr;
				return true;
			}
		}

		if (!inline_node)
		{
			// a message the server delivered, or one only the server handles: received through it, in order.
			// it answers DSQ_RCV_WAIT_LOCALLY if the queue is empty by then
			mach_msg_return_t mr = sq_server_receive(rcv_addr, option, rcv_size, rcv_name,
				(option & MACH_RCV_TIMEOUT) && have_deadline ? sq_remaining(&deadline) : timeout, true);
			if (mr == DSQ_RCV_WAIT_LOCALLY)
				continue;
			// the server leaves a message too large for a MACH_RCV_LARGE receive queued, too
			if (mr == MACH_RCV_TOO_LARGE && (option & MACH_RCV_LARGE))
				sq_pass_wake(&qslot->rcv_waiters, &qslot->head);
			*result = mr;
			return true;
		}

		struct sq_defer defer;
		uint32_t seqno;
		sq_defer_begin(&defer);
		// a MAKE_SEND message is counted in the same CAS that takes it (the port's make-send count)
		if (!dsq_take(&sq_view, entry.slot, d, (flags & DSQ_NODE_MADE_SEND) != 0, &seqno))
		{
			sq_defer_end(&defer);
			continue;
		}
		if (__atomic_load_n(&qslot->snd_waiters, __ATOMIC_ACQUIRE) != 0)
			sq_futex_wake(&qslot->deq, 1);
		sq_defer_end(&defer);

		// the node is ours now
		uint8_t out[DSQ_MSG_MAX + DSQ_TRAILER_MAX];
		uint32_t sec_token[2], audit_token[8];
		uint64_t context = __atomic_load_n(&sslot->context, __ATOMIC_ACQUIRE);
		uint32_t length;

		dsq_tokens_lookup(&sq_view, node->aux, sec_token, audit_token);
		node->size = (uint16_t) size;
		length = dsq_build_received(node, rcv_name, option, seqno, context, sec_token, audit_token, out);
		dsq_task_node_free(&sq_view, index);

		// a bad buffer loses the message, as in XNU
		*result = sq_copyout(rcv_addr, out, length) ? MACH_MSG_SUCCESS : MACH_RCV_INVALID_DATA;
		return true;
	}
}


// A receive on a port set whose members all keep their queues in the region: the head of the member that has been
// ready longest (dsq_set_select()). The message is the member's, with the member's name, seqno and context.
static bool sq_set_receive(dsq_name_entry_t entry, void* rcv_addr, mach_msg_option_t option, mach_msg_size_t rcv_size,
		mach_port_name_t rcv_name, mach_msg_timeout_t timeout, mach_msg_return_t* result)
{
	struct sq_timespec deadline;
	bool have_deadline = false;
	bool expired = false;
	uint32_t trailer_size = dsq_trailer_size(option);

	dsq_q_slot_t* qslot = &sq_view.q->slots[entry.slot];
	const dsq_s_slot_t* sslot = &sq_view.s->slots[entry.slot];
	const uint32_t gen = __atomic_load_n(&sslot->gen, __ATOMIC_ACQUIRE);
	unsigned int stale = 0;

	for (unsigned int round = 0;; round++)
	{
		dsq_set_pick_t pick;
		int r;
		// before looking: a publish after this changes what the wait below compares with
		uint32_t word = __atomic_load_n(&qslot->setword, __ATOMIC_SEQ_CST);

		if (round > 1000000)
			return false;

		// from here to the take, the server's removal of a member sees this receive (dsq_set_member_remove())
		dsq_set_enter(&sq_view, entry.slot);
		r = dsq_set_select(&sq_view, entry.slot, entry.epoch, &pick);

		if (r == DSQ_RETRY)
		{
			dsq_set_exit(&sq_view, entry.slot);
			continue;
		}
		if (r == DSQ_STALE)
		{
			dsq_set_exit(&sq_view, entry.slot);
			if (sq_receive_refresh(rcv_name, &entry, gen) && ++stale < 100000)
				continue;
			*result = sq_server_receive(rcv_addr, option, rcv_size, rcv_name,
				(option & MACH_RCV_TIMEOUT) && have_deadline ? sq_remaining(&deadline) : timeout, false);
			return true;
		}
		if (r == DSQ_CLOSED_Q)
		{
			dsq_set_exit(&sq_view, entry.slot);
			uint32_t reason = __atomic_load_n(&sslot->close_reason, __ATOMIC_ACQUIRE);
			// what XNU's blocked receivers get when the set is destroyed, if the close was this call's set's:
			// the slot may have been given to another set or port since, and closed again with its reason
			// (the server changes gen before it writes a new reason)
			if ((reason == DSQ_CLOSE_MOVED || reason == DSQ_CLOSE_DESTROYED) &&
					__atomic_load_n(&sslot->gen, __ATOMIC_ACQUIRE) == gen)
			{
				*result = MACH_RCV_PORT_CHANGED;
				return true;
			}
			*result = sq_server_receive(rcv_addr, option, rcv_size, rcv_name,
				(option & MACH_RCV_TIMEOUT) && have_deadline ? sq_remaining(&deadline) : timeout, false);
			return true;
		}
		if (r == DSQ_CORRUPT || !sq_ready())
		{
			dsq_set_exit(&sq_view, entry.slot);
			return false;
		}

		if (r == DSQ_EMPTY)
		{
			long ret = 0;
			uint64_t h;

			dsq_set_exit(&sq_view, entry.slot);
			if (((option & MACH_RCV_TIMEOUT) && timeout == 0) || expired)
			{
				*result = MACH_RCV_TIMED_OUT;
				return true;
			}
			if (!have_deadline)
			{
				sq_deadline((option & MACH_RCV_TIMEOUT) != 0, timeout, &deadline);
				have_deadline = true;
			}

			__atomic_add_fetch(&qslot->rcv_waiters, 1, __ATOMIC_SEQ_CST);
			h = dsq_load(&qslot->head);
			// the wait is only for this call's set: a head that is closed or has another epoch is looked at from the top
			if (!DSQ_IS_CLOSED(h) && DSQ_H_EPOCH(h) == entry.epoch)
				ret = sq_futex_wait((uint64_t*) &qslot->setword, word, &deadline);
			__atomic_sub_fetch(&qslot->rcv_waiters, 1, __ATOMIC_SEQ_CST);

			if (ret == -LINUX_ETIMEDOUT)
				expired = true; // one more look before giving up
			else if (ret == -LINUX_EINTR && (option & MACH_RCV_INTERRUPT))
			{
				*result = MACH_RCV_INTERRUPTED;
				return true;
			}
			continue;
		}

		// the head of the member that was picked
		const dsq_node_t* node = dsq_node(&sq_view, pick.index);
		const uint32_t flags = pick.flags;
		const uint32_t size = pick.size;
		bool inline_node = pick.index < DSQ_TNODES && (flags & DSQ_NODE_INLINE) != 0 &&
			size >= DSQ_MSG_MIN && size <= DSQ_MSG_MAX;
		const dsq_s_slot_t* msslot = &sq_view.s->slots[pick.member];
		uint32_t member_name = __atomic_load_n(&msslot->name, __ATOMIC_ACQUIRE);
		uint64_t context = __atomic_load_n(&msslot->context, __ATOMIC_ACQUIRE);

		if (inline_node && size + trailer_size > rcv_size)
		{
			if ((option & MACH_RCV_LARGE) == 0)
				inline_node = false; // the server destroys it, as XNU does
			else
			{
				// left queued; the caller learns the size it needs, and with LARGE_IDENTITY the member's name
				mach_msg_return_t mr = MACH_RCV_TOO_LARGE;
				dsq_set_exit(&sq_view, entry.slot);
				if (rcv_size >= 16)
				{
					if ((option & MACH_RCV_LARGE_IDENTITY) &&
							!sq_copyout((char*) rcv_addr + 12, &member_name, sizeof(member_name)))
						mr = MACH_RCV_INVALID_DATA;
					if (!sq_copyout((char*) rcv_addr + 4, &size, sizeof(size)))
						mr = MACH_RCV_INVALID_DATA;
				}
				sq_pass_wake(&qslot->rcv_waiters, (uint64_t*) &qslot->setword);
				*result = mr;
				return true;
			}
		}

		if (!inline_node)
		{
			// a message the server delivered, or one only the server handles: received through it, in order.
			// it answers DSQ_RCV_WAIT_LOCALLY if there is nothing left by then
			dsq_set_exit(&sq_view, entry.slot);
			mach_msg_return_t mr = sq_server_receive(rcv_addr, option, rcv_size, rcv_name,
				(option & MACH_RCV_TIMEOUT) && have_deadline ? sq_remaining(&deadline) : timeout, true);
			if (mr == DSQ_RCV_WAIT_LOCALLY)
				continue;
			if (mr == MACH_RCV_TOO_LARGE && (option & MACH_RCV_LARGE))
				sq_pass_wake(&qslot->rcv_waiters, (uint64_t*) &qslot->setword);
			*result = mr;
			return true;
		}

		struct sq_defer defer;
		uint32_t seqno;
		bool taken;
		sq_defer_begin(&defer);
		taken = dsq_set_take(&sq_view, entry.slot, &pick, (flags & DSQ_NODE_MADE_SEND) != 0, &seqno);
		dsq_set_exit(&sq_view, entry.slot);
		if (!taken)
		{
			sq_defer_end(&defer);
			continue;
		}
		if (__atomic_load_n(&sq_view.q->slots[pick.member].snd_waiters, __ATOMIC_ACQUIRE) != 0)
			sq_futex_wake(&sq_view.q->slots[pick.member].deq, 1);
		sq_defer_end(&defer);

		// the node is ours now
		uint8_t out[DSQ_MSG_MAX + DSQ_TRAILER_MAX];
		uint32_t sec_token[2], audit_token[8];
		uint32_t length;

		dsq_tokens_lookup(&sq_view, pick.aux, sec_token, audit_token);
		((dsq_node_t*) node)->size = (uint16_t) size;
		length = dsq_build_received(node, member_name, option, seqno, context, sec_token, audit_token, out);
		dsq_task_node_free(&sq_view, pick.index);

		// a bad buffer loses the message, as in XNU
		*result = sq_copyout(rcv_addr, out, length) ? MACH_MSG_SUCCESS : MACH_RCV_INVALID_DATA;
		return true;
	}
}

// ---- mach_msg ------------------------------------------------------------------------

bool mach_sq_msg(mach_msg_header_t* msg, mach_msg_option_t option, mach_msg_size_t send_size,
		mach_msg_size_t rcv_size, mach_port_name_t rcv_name, mach_msg_timeout_t timeout,
		mach_msg_header_t* rcv_msg, mach_msg_return_t* result)
{
	mach_msg_option_t lib_options = option & (MACH_SEND_INTERRUPT | MACH_RCV_INTERRUPT);
	void* rcv_addr = (rcv_msg != NULL) ? (void*) rcv_msg : (void*) msg;

	if (!sq_ready())
		return false;

	// the server ignores everything else
	option = (option & SQ_OPTION_USER) | lib_options;

	if (option & MACH_SEND_MSG)
	{
		bool interrupted = false;
		int sent = sq_send(msg, option, send_size, timeout, &interrupted);

		if (sent == SQ_SEND_SERVER)
			return false;

		if (sent == SQ_SEND_FULL)
		{
			// the server finishes the send without waiting: it sends the message if there is room by now,
			// and otherwise times out and gives the rights back to the sender
			mach_msg_return_t mr;
			int code;

			do
				code = dserver_rpc_mach_msg_overwrite(msg, (option & ~(MACH_RCV_MSG | MACH_RCV_INTERRUPT)) | MACH_SEND_TIMEOUT,
					send_size, 0, MACH_PORT_NULL, 0, MACH_PORT_NULL, NULL);
			while (code == -DSERVER_RPC_RESTART || code == -LINUX_EINTR);
			if (code < 0)
			{
				__simple_printf("mach_msg_overwrite failed (internally): %d\n", code);
				__simple_abort();
			}
			mr = code;
			if (mr == MACH_SEND_TIMED_OUT && interrupted)
				mr = MACH_SEND_INTERRUPTED;
			if (mr != MACH_MSG_SUCCESS || (option & MACH_RCV_MSG) == 0)
			{
				*result = mr;
				return true;
			}
		}
		else if ((option & MACH_RCV_MSG) == 0)
		{
			*result = MACH_MSG_SUCCESS;
			return true;
		}

		// the send is done; the receive follows on its own
		option &= ~(MACH_SEND_MSG | MACH_SEND_TIMEOUT | MACH_SEND_NOIMPORTANCE | MACH_SEND_INTERRUPT);
		if (!sq_receive(rcv_addr, option, rcv_size, rcv_name, timeout, result))
			*result = sq_server_receive(rcv_addr, option, rcv_size, rcv_name, timeout, false);
		return true;
	}

	if (option & MACH_RCV_MSG)
		return sq_receive(rcv_addr, option, rcv_size, rcv_name, timeout, result);

	return false;
}

#endif // !VARIANT_DYLD
