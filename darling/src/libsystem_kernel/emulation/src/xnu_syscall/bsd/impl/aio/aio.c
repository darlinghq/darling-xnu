// POSIX asynchronous I/O (aio_read, aio_write, aio_fsync, aio_error, aio_return, aio_suspend, aio_cancel,
// lio_listio; <aio.h>, aio(4)-style semantics, BSD syscalls 313-320 and 421).
//
// Requests complete synchronously when they are submitted (a pread/pwrite/fsync in the calling thread): the result
// then waits in the table below for aio_error/aio_return, as a request on a done queue does. Where behaviour is not
// in the public documentation it follows black-box observation of macOS 26.7.3 (kern.aioprocmax is 16 on Mac OS X 10.14.4 too):
//   - a process has at most AIO_PROCESS_MAX (16, the value of kern.aioprocmax on both) requests submitted and not
//     yet reaped by aio_return; more fail with EAGAIN. That also bounds the table: a caller that never calls
//     aio_return gets EAGAIN, not a leak;
//   - an invalid request fails its submission with its own errno: EBADF (descriptor not open, or not open for that
//     access), ESPIPE (pipe, socket), EINVAL (length, buffer, offset, sigevent, lio opcode); lio_listio then submits
//     nothing. An I/O error is the request's error (aio_error), with aio_return 0. LIO_NOP completes with 0;
//   - aio_error / aio_return of a request that is not (or no longer) known fail with EINVAL;
//   - SIGEV_SIGNAL sends the signal to the process on completion (for lio_listio, once, after the last request);
//     SIGEV_THREAD is EINVAL. There is no EVFILT_AIO;
//   - aio_suspend returns at once if a listed request is done, EINVAL if the process has no requests at all, else
//     waits (in short sleeps, interruptible) for another thread to submit one, until its timeout (EAGAIN);
//   - aio_cancel finds no request in flight (they complete at submission): AIO_ALLDONE for a done one.
// macOS completes requests asynchronously, so aio_error can answer EINPROGRESS right after a submission; here the
// request is already done, which the contract allows (a request may complete at any time).
// The aiocb is the native layout of the process's architecture (LP64 on x86_64, ILP32 on i386).
//
// Tests: tests/src/libsystem_kernel_aio.c

#include <darling/emulation/xnu_syscall/bsd/impl/aio/aio.h>

#include <stddef.h>
#include <stdint.h>
#include <sys/errno.h>
#include <libsimple/lock.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/conversion/duct_errno.h>
#include <darling/emulation/conversion/signal/duct_signals.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/xnu_syscall/bsd/helper/bsdthread/cancelable.h>

#define AIO_PROCESS_MAX 16 // kern.aioprocmax
#define AIO_LISTIO_MAX 16

#define XNU_AIO_ALLDONE 0x1
#define XNU_LIO_NOP 0x0
#define XNU_LIO_READ 0x1
#define XNU_LIO_WRITE 0x2
#define XNU_LIO_NOWAIT 0x1
#define XNU_LIO_WAIT 0x2
#define XNU_SIGEV_NONE 0
#define XNU_SIGEV_SIGNAL 1
#define XNU_NSIG 32
#define XNU_SIGKILL 9
#define XNU_SIGSTOP 17
#define XNU_O_SYNC 0x80
#define XNU_O_DSYNC 0x400000

#define LINUX_F_GETFL 3
#define LINUX_O_ACCMODE 03
#define LINUX_O_RDONLY 00
#define LINUX_O_WRONLY 01
#define LINUX_O_PATH 010000000
#define LINUX_AT_EMPTY_PATH 0x1000
#define LINUX_STATX_TYPE 0x1
#define LINUX_S_IFMT 0170000
#define LINUX_S_IFIFO 0010000
#define LINUX_S_IFSOCK 0140000

// <sys/signal.h> struct sigevent and <sys/aio.h> struct aiocb, compiled per architecture like the guest's
struct xnu_sigevent
{
	int sigev_notify;
	int sigev_signo;
	void* sigev_value;
	void (*sigev_notify_function)(void*);
	void* sigev_notify_attributes;
};

struct xnu_aiocb
{
	int aio_fildes;
	long long aio_offset;
	volatile void* aio_buf;
	unsigned long aio_nbytes;
	int aio_reqprio;
	struct xnu_sigevent aio_sigevent;
	int aio_lio_opcode;
};

enum op { OP_NOP, OP_READ, OP_WRITE, OP_FSYNC, OP_DSYNC };

// A request from submission to aio_return. `aiocbp` NULL: a free slot; `done` 0: being performed by its submitter.
struct request
{
	const void* aiocbp;
	int fd;
	int done;
	int error;
	long result;
};

static struct request requests[AIO_PROCESS_MAX];
static int nrequests; // slots in use
static libsimple_lock_t aio_lock = LIBSIMPLE_LOCK_INITIALIZER;
static volatile unsigned int completions; // bumped on every completion, for aio_suspend

void aio_postfork_child(void)
{
	for (int i = 0; i < AIO_PROCESS_MAX; i++)
		requests[i].aiocbp = NULL;
	nrequests = 0;
	libsimple_lock_init(&aio_lock);
}

static int sigev_valid(const struct xnu_sigevent* sev)
{
	switch (sev->sigev_notify)
	{
		case XNU_SIGEV_SIGNAL:
			return sev->sigev_signo > 0 && sev->sigev_signo < XNU_NSIG
				&& sev->sigev_signo != XNU_SIGKILL && sev->sigev_signo != XNU_SIGSTOP;
		case XNU_SIGEV_NONE:
			return 1;
		default: // SIGEV_THREAD is unsupported on XNU too
			return 0;
	}
}

// statx's leading fields, the same on every architecture
struct linux_statx_head
{
	uint32_t stx_mask, stx_blksize;
	uint64_t stx_attributes;
	uint32_t stx_nlink, stx_uid, stx_gid;
	uint16_t stx_mode;
};

// aio_validate: the descriptor is open for the access op needs (EBADF) and is not a pipe or socket (ESPIPE)
static int fd_check(int fd, enum op op)
{
	long fl = LINUX_SYSCALL(__NR_fcntl, fd, LINUX_F_GETFL, 0);
	if (fl < 0 || (fl & LINUX_O_PATH))
		return EBADF;
	int acc = fl & LINUX_O_ACCMODE;
	if (op == OP_READ ? acc == LINUX_O_WRONLY : acc == LINUX_O_RDONLY)
		return EBADF;

	union { struct linux_statx_head h; char buf[256]; } stx;
	if (LINUX_SYSCALL(__NR_statx, fd, "", LINUX_AT_EMPTY_PATH, LINUX_STATX_TYPE, &stx) < 0)
		return EBADF;
	int fmt = stx.h.stx_mode & LINUX_S_IFMT;
	return fmt == LINUX_S_IFIFO || fmt == LINUX_S_IFSOCK ? ESPIPE : 0;
}

// aio_create_queue_entry + aio_validate: 0 or the errno the submission fails with
static int request_check(const struct xnu_aiocb* cb, enum op op)
{
	if (op == OP_NOP)
		return 0;
	if (op == OP_READ || op == OP_WRITE)
	{
		if (cb->aio_nbytes > 0x7fffffff || cb->aio_buf == NULL || cb->aio_offset < 0)
			return EINVAL;
	}
	if (!sigev_valid(&cb->aio_sigevent))
		return EINVAL;
	return fd_check(cb->aio_fildes, op);
}

// Takes a slot for aiocbp, or returns NULL when the process has AIO_PROCESS_MAX requests (EAGAIN)
static struct request* request_reserve(const void* aiocbp, int fd)
{
	struct request* r = NULL;
	libsimple_lock_lock(&aio_lock);
	if (nrequests < AIO_PROCESS_MAX)
	{
		for (int i = 0; i < AIO_PROCESS_MAX; i++)
		{
			if (requests[i].aiocbp == NULL)
			{
				r = &requests[i];
				r->aiocbp = aiocbp;
				r->fd = fd;
				r->done = 0;
				r->error = 0;
				r->result = 0;
				nrequests++;
				break;
			}
		}
	}
	libsimple_lock_unlock(&aio_lock);
	return r;
}

// do_aio_read / do_aio_write / do_aio_fsync. Returns the BSD errno (0: success, result in *result).
static int perform(const struct xnu_aiocb* cb, enum op op, long* result)
{
	long ret;

	*result = 0;
	switch (op)
	{
		case OP_READ:
			ret = LINUX_SYSCALL(__NR_pread64, cb->aio_fildes, cb->aio_buf, cb->aio_nbytes, LL_ARG(cb->aio_offset));
			break;
		case OP_WRITE:
			// with O_APPEND, Linux's pwrite appends like XNU's (which ignores the offset then)
			ret = LINUX_SYSCALL(__NR_pwrite64, cb->aio_fildes, cb->aio_buf, cb->aio_nbytes, LL_ARG(cb->aio_offset));
			break;
		case OP_FSYNC:
			ret = LINUX_SYSCALL(__NR_fsync, cb->aio_fildes);
			break;
		case OP_DSYNC:
			ret = LINUX_SYSCALL(__NR_fdatasync, cb->aio_fildes);
			break;
		default: // LIO_NOP
			return 0;
	}
	if (ret < 0)
		return -errno_linux_to_bsd(ret);
	*result = (op == OP_READ || op == OP_WRITE) ? ret : 0;
	return 0;
}

static void request_complete(struct request* r, int error, long result)
{
	libsimple_lock_lock(&aio_lock);
	r->error = error;
	r->result = result;
	r->done = 1;
	__atomic_add_fetch(&completions, 1, __ATOMIC_RELEASE);
	libsimple_lock_unlock(&aio_lock);
}

// psignal(p, signo): to the process, not the thread
static void notify(const struct xnu_sigevent* sev)
{
	if (sev->sigev_notify != XNU_SIGEV_SIGNAL)
		return;
	int linux_signo = signum_bsd_to_linux(sev->sigev_signo);
	if (linux_signo > 0)
		LINUX_SYSCALL(__NR_kill, LINUX_SYSCALL0(__NR_getpid), linux_signo);
}

static enum op lio_op(int opcode)
{
	return opcode == XNU_LIO_READ ? OP_READ : opcode == XNU_LIO_WRITE ? OP_WRITE : OP_NOP;
}

// aio_queue_async_request plus the work, done now
static long submit(void* aiocbp, enum op op)
{
	struct xnu_aiocb cb;
	struct request* r;
	long result;
	int error;

	if (aiocbp == NULL)
		return -EFAULT;
	cb = *(const struct xnu_aiocb*) aiocbp;
	error = request_check(&cb, op);
	if (error != 0)
		return -error;
	r = request_reserve(aiocbp, cb.aio_fildes);
	if (r == NULL)
		return -EAGAIN;

	error = perform(&cb, op, &result);
	request_complete(r, error, result);
	notify(&cb.aio_sigevent);
	return 0;
}

long sys_aio_read(void* aiocbp)
{
	return submit(aiocbp, OP_READ);
}

long sys_aio_write(void* aiocbp)
{
	return submit(aiocbp, OP_WRITE);
}

long sys_aio_fsync(int op, void* aiocbp)
{
	if (op == XNU_O_SYNC || op == 0)
		return submit(aiocbp, OP_FSYNC);
	if (op == XNU_O_DSYNC)
		return submit(aiocbp, OP_DSYNC);
	return -EINVAL;
}

// The first request for aiocbp (XNU keeps a done queue in completion order; ours completes in submission order)
static struct request* request_find_locked(const void* aiocbp)
{
	for (int i = 0; i < AIO_PROCESS_MAX; i++)
		if (requests[i].aiocbp != NULL && requests[i].aiocbp == aiocbp)
			return &requests[i];
	return NULL;
}

long sys_aio_error(void* aiocbp)
{
	long ret = -EINVAL;
	libsimple_lock_lock(&aio_lock);
	struct request* r = aiocbp != NULL ? request_find_locked(aiocbp) : NULL;
	if (r != NULL)
		ret = r->done ? r->error : EINPROGRESS;
	libsimple_lock_unlock(&aio_lock);
	return ret;
}

long sys_aio_return(void* aiocbp)
{
	long ret = -EINVAL;
	libsimple_lock_lock(&aio_lock);
	struct request* r = aiocbp != NULL ? request_find_locked(aiocbp) : NULL;
	if (r != NULL)
	{
		if (r->done)
		{
			ret = r->result;
			r->aiocbp = NULL;
			nrequests--;
		}
		else
			ret = -EINPROGRESS;
	}
	libsimple_lock_unlock(&aio_lock);
	return ret;
}

long sys_aio_cancel(int fd, void* aiocbp)
{
	long ret;
	
	if (aiocbp != NULL && ((const struct xnu_aiocb*) aiocbp)->aio_fildes != fd)
		return -EBADF;

	libsimple_lock_lock(&aio_lock);
	if (nrequests == 0)
		ret = XNU_AIO_ALLDONE;
	else
	{
		ret = -EBADF; // do_aio_cancel_locked found nothing
		for (int i = 0; i < AIO_PROCESS_MAX; i++)
		{
			const struct request* r = &requests[i];
			if (r->aiocbp == NULL || (aiocbp != NULL ? r->aiocbp != aiocbp : r->fd != fd))
				continue;
			// a request still being performed by its submitter can't be cancelled (AIO_NOTCANCELED on XNU,
			// but the caller's aiocb is done the moment the submitter returns): report it done like the rest
			ret = XNU_AIO_ALLDONE;
			break;
		}
	}
	libsimple_lock_unlock(&aio_lock);
	return ret;
}

// Whether a request in the list is done; -1 when the process has no requests at all (EINVAL on XNU)
static int any_done(void* const* list, int nent)
{
	int ret = 0;
	libsimple_lock_lock(&aio_lock);
	if (nrequests == 0)
		ret = -1;
	for (int i = 0; i < nent && ret == 0; i++)
	{
		if (list[i] == NULL)
			continue;
		for (int k = 0; k < AIO_PROCESS_MAX; k++)
		{
			if (requests[k].aiocbp == list[i] && requests[k].done)
			{
				ret = 1;
				break;
			}
		}
	}
	libsimple_lock_unlock(&aio_lock);
	return ret;
}

struct xnu_timespec
{
	long tv_sec;
	long tv_nsec;
};

static long aio_suspend_common(void* const* list, int nent, const void* timeoutp, int cancelable)
{
	long long left_ns = -1; // no timeout
	long long step_ns = 1000000;

	if (nent < 1 || nent > AIO_PROCESS_MAX)
		return -EINVAL;
	if (timeoutp != NULL)
	{
		const struct xnu_timespec* ts = timeoutp;
		if (ts->tv_sec < 0 || ts->tv_nsec < 0 || ts->tv_nsec >= 1000000000)
			return -EINVAL;
		left_ns = (long long) ts->tv_sec * 1000000000LL + ts->tv_nsec;
	}

	for (;;)
	{
		unsigned int seen = __atomic_load_n(&completions, __ATOMIC_ACQUIRE);
		int done = any_done(list, nent);
		if (done < 0)
			return -EINVAL;
		if (done > 0)
			return 0;
		if (left_ns == 0)
			return -EAGAIN;

		// Nothing listed is done: only another thread's submission can change that. Sleep a little (to the
		// timeout at most), 1 ms first, backing off to 50 ms; a signal ends the wait with EINTR as on XNU.
		long long sleep_ns = step_ns;
		if (left_ns >= 0 && sleep_ns > left_ns)
			sleep_ns = left_ns;
		if (__atomic_load_n(&completions, __ATOMIC_ACQUIRE) != seen)
			continue;
		struct xnu_timespec req = { sleep_ns / 1000000000LL, sleep_ns % 1000000000LL };
		long ret = LINUX_SYSCALL_C(cancelable, __NR_nanosleep, &req, NULL);
		if (ret == -LINUX_EINTR)
			return -EINTR;
		if (left_ns > 0)
			left_ns -= sleep_ns;
		if (step_ns < 50000000)
			step_ns *= 2;
	}
}

long sys_aio_suspend(void* const* aiocblist, int nent, const void* timeoutp)
{
	CANCELATION_POINT();
	return aio_suspend_common(aiocblist, nent, timeoutp, 1);
}

long sys_aio_suspend_nocancel(void* const* aiocblist, int nent, const void* timeoutp)
{
	return aio_suspend_common(aiocblist, nent, timeoutp, 0);
}

long sys_lio_listio(int mode, void* const* aiocblist, int nent, const void* sigp)
{
	struct xnu_sigevent sev = { XNU_SIGEV_NONE };
	struct xnu_aiocb cbs[AIO_LISTIO_MAX];
	void* ptrs[AIO_LISTIO_MAX];
	struct request* reqs[AIO_LISTIO_MAX];
	int count = 0;
	long ret = 0;

	if (mode != XNU_LIO_NOWAIT && mode != XNU_LIO_WAIT)
		return -EINVAL;
	if (nent < 1 || nent > AIO_LISTIO_MAX)
		return -EINVAL;
	if (sigp != NULL)
	{
		sev = *(const struct xnu_sigevent*) sigp;
		if (!sigev_valid(&sev))
			return -EINVAL;
	}
	if (aiocblist == NULL)
		return -EFAULT;

	// check every entry first: an invalid one fails the call with its errno, nothing submitted (macOS 27)
	for (int i = 0; i < nent; i++)
	{
		if (aiocblist[i] == NULL)
			continue;
		cbs[count] = *(const struct xnu_aiocb*) aiocblist[i];
		int opcode = cbs[count].aio_lio_opcode;
		if (opcode != XNU_LIO_NOP && opcode != XNU_LIO_READ && opcode != XNU_LIO_WRITE)
			return -EINVAL;
		int error = request_check(&cbs[count], lio_op(opcode));
		if (error != 0)
			return -error;
		ptrs[count] = aiocblist[i];
		count++;
	}
	if (count == 0)
		return 0;

	// submit what fits (XNU enqueues each it can and returns EAGAIN if one didn't)
	for (int i = 0; i < count; i++)
	{
		reqs[i] = request_reserve(ptrs[i], cbs[i].aio_fildes);
		if (reqs[i] == NULL)
			ret = -EAGAIN;
	}

	const struct xnu_sigevent* last_sev = NULL;
	for (int i = 0; i < count; i++)
	{
		if (reqs[i] == NULL)
			continue;
		long result;
		int error = perform(&cbs[i], lio_op(cbs[i].aio_lio_opcode), &result);
		request_complete(reqs[i], error, result);
		// LIO_NOWAIT: the call's sigevent replaces each entry's; LIO_WAIT: entries keep their own. Either way
		// only the last completion of the group signals.
		last_sev = mode == XNU_LIO_NOWAIT ? &sev : &cbs[i].aio_sigevent;
	}
	if (last_sev != NULL)
		notify(last_sev);

	return ret;
}
