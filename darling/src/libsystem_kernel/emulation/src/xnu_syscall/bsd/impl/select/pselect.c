#include <darling/emulation/xnu_syscall/bsd/impl/select/pselect.h>

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/xnu_syscall/bsd/helper/bsdthread/cancelable.h>
#include <darling/emulation/linux_premigration/misc/ioctl.h>

#define DARLING_SELECT_FD_SETSIZE 1024
#define LINUX_POLLHUP 0x0010
#define LINUX_POLLIN 0x0001
#define LINUX_POLLPRI 0x0002
#define LINUX_POLLOUT 0x0004
#define LINUX_POLLERR 0x0008
#define LINUX_POLLNVAL 0x0020
#define LINUX_POLLRDNORM 0x0040
#define LINUX_POLLRDBAND 0x0080
#define LINUX_POLLWRNORM 0x0100
#define LINUX_POLLWRBAND 0x0200
#define LINUX_EBADF 9
#define LINUX_TCGETA 0x5405

struct linux_pollfd {
	int fd;
	short events;
	short revents;
};

struct linux_timespec {
	long tv_sec;
	long tv_nsec;
};

static int fd_is_set(int fd, const uint32_t* set)
{
	return (set[fd / 32] & (1u << (fd % 32))) != 0;
}

static void fd_set_bit(int fd, uint32_t* set)
{
	set[fd / 32] |= 1u << (fd % 32);
}

static int add_pty_hangups_to_exception_set(int nfds, const uint32_t* requested,
		uint32_t* returned)
{
	int added = 0;
	struct linux_timespec zero = { 0, 0 };

	for (int fd = 0; fd < nfds; ++fd) {
		if (!fd_is_set(fd, requested) || fd_is_set(fd, returned))
			continue;

		struct linux_pollfd pollfd = { .fd = fd };
		int poll_result = LINUX_SYSCALL(__NR_ppoll, &pollfd, 1, &zero, NULL, 0);
		if (poll_result <= 0 || !(pollfd.revents & LINUX_POLLHUP))
			continue;

		char termios[18];
		if (__real_ioctl(fd, LINUX_TCGETA, termios) != 0)
			continue;

		fd_set_bit(fd, returned);
		++added;
	}
	return added;
}

long sys_pselect(int nfds, void* rfds, void* wfds, void* efds, const struct bsd_timespec* timeout, const sigset_t* mask)
{
	CANCELATION_POINT();
	return sys_pselect_nocancel(nfds, rfds, wfds, efds, timeout, mask);
}

struct pselect_epoll_event {
	uint32_t events;
	uint64_t data;
}
#if defined(__x86_64__)
__attribute__((packed))
#endif
;

/* Keep ordinary descriptors in native pselect; epoll supplies PTY wakeups. */
static long pselect_pty_wait(int nfds, uint32_t* rfds, uint32_t* wfds,
		uint32_t* efds, const struct bsd_timespec* timeout, const sigset_t* mask,
		const unsigned char* tty)
{
	/* Do not let the internal descriptor occupy a requested invalid fd. */
	for (int fd = 0; fd < nfds; ++fd) {
		if ((rfds && fd_is_set(fd, rfds)) || (wfds && fd_is_set(fd, wfds)) || fd_is_set(fd, efds)) {
			long valid = LINUX_SYSCALL(__NR_fcntl, fd, 1, 0); /* F_GETFD */
			if (valid < 0) return errno_linux_to_bsd(valid);
		}
	}
	long ep = LINUX_SYSCALL(__NR_epoll_create1, 0x80000); /* EPOLL_CLOEXEC */
	if (ep < 0) return errno_linux_to_bsd(ep);
	long result;
	int limit = ep >= nfds ? ep + 1 : nfds;
	size_t set_bytes = ((size_t)limit + 63) / 64 * 8;
	void* large_sets = NULL;
	if (ep >= DARLING_SELECT_FD_SETSIZE) {
#ifdef __NR_mmap2
		long memory = LINUX_SYSCALL(__NR_mmap2, NULL, set_bytes * 3, 3, 0x22, -1, 0);
#else
		long memory = LINUX_SYSCALL(__NR_mmap, NULL, set_bytes * 3, 3, 0x22, -1, 0);
#endif
		if ((unsigned long)memory >= (unsigned long)-4095) { result = memory; goto cleanup; }
		large_sets = (void*)memory;
	}
	for (int fd = 0; fd < nfds; ++fd) {
		if (!tty[fd]) continue;
		struct pselect_epoll_event event = { .events = LINUX_POLLPRI | (1u << 31), .data = fd };
		result = LINUX_SYSCALL(__NR_epoll_ctl, ep, 1, fd, &event); /* ADD, edge triggered */
		if (result < 0) goto cleanup;
	}
	struct linux_timespec remaining, *wait = NULL;
	if (timeout) {
		remaining.tv_sec = timeout->tv_sec;
		remaining.tv_nsec = timeout->tv_nsec;
		wait = &remaining;
	}
	linux_sigset_t lmask;
	long mask_data[2];
	if (mask) {
		sigset_bsd_to_linux(mask, &lmask);
		mask_data[0] = (long)&lmask; mask_data[1] = sizeof(lmask);
	}
	size_t bytes = (size_t)((nfds + 31) / 32) * sizeof(uint32_t);
	for (;;) {
		uint32_t small_sets[3][DARLING_SELECT_FD_SETSIZE / 32] = {{0}};
		uint32_t* reads = large_sets ? large_sets : small_sets[0];
		uint32_t* writes = large_sets ? (uint32_t*)((char*)large_sets + set_bytes) : small_sets[1];
		uint32_t* exceptions = large_sets ? (uint32_t*)((char*)large_sets + 2 * set_bytes) : small_sets[2];
		if (large_sets) memset(large_sets, 0, set_bytes * 3);
		if (rfds) memcpy(reads, rfds, bytes);
		if (wfds) memcpy(writes, wfds, bytes);
		memcpy(exceptions, efds, bytes);
		/* Raising nfds for ep must not expose caller padding bits. */
		if (nfds % 32) {
			uint32_t valid = (1u << (nfds % 32)) - 1;
			reads[nfds / 32] &= valid;
			writes[nfds / 32] &= valid;
			exceptions[nfds / 32] &= valid;
		}
		fd_set_bit(ep, reads);
		result = LINUX_SYSCALL(__NR_pselect6, limit, reads, wfds ? writes : NULL,
				exceptions, wait, mask ? mask_data : NULL);
		if (result < 0) goto cleanup;
		if (fd_is_set(ep, reads)) {
			reads[ep / 32] &= ~(1u << (ep % 32));
			--result;
			for (;;) {
				struct pselect_epoll_event event;
				long count = LINUX_SYSCALL(__NR_epoll_pwait, ep, &event, 1, 0, NULL, sizeof(lmask));
				if (count < 0) { result = count; goto cleanup; }
				if (!count) break;
				int fd = event.data;
				if ((event.events & (LINUX_POLLHUP | LINUX_POLLPRI)) && !fd_is_set(fd, exceptions)) {
					fd_set_bit(fd, exceptions); ++result;
				}
			}
			if (!result) continue; /* Unrequested error; keep every caller fd watched. */
		}
		if (rfds) memcpy(rfds, reads, bytes);
		if (wfds) memcpy(wfds, writes, bytes);
		memcpy(efds, exceptions, bytes);
		break;
	}
cleanup:
	if (large_sets) LINUX_SYSCALL(__NR_munmap, large_sets, set_bytes * 3);
	LINUX_SYSCALL(__NR_close, ep);
	return result < 0 ? errno_linux_to_bsd(result) : result;
}

long sys_pselect_nocancel(int nfds, void* rfds, void* wfds, void* efds, const struct bsd_timespec* timeout, const sigset_t* mask)
{
	if (efds && nfds > 0 && nfds <= DARLING_SELECT_FD_SETSIZE) {
		unsigned char tty[DARLING_SELECT_FD_SETSIZE] = {0};
		int has_tty = 0;
		for (int fd = 0; fd < nfds; ++fd) {
			char termios[18];
			if (fd_is_set(fd, efds) && __real_ioctl(fd, LINUX_TCGETA, termios) == 0)
				has_tty = tty[fd] = 1;
		}
		if (has_tty) return pselect_pty_wait(nfds, rfds, wfds, efds, timeout, mask, tty);
	}
	int ret;
	struct linux_timespec { long tv_sec; long tv_nsec; } lts;
	long data[2];
	linux_sigset_t lmask;
	uint32_t requested_exceptions[DARLING_SELECT_FD_SETSIZE / 32];
	uint32_t pending_pty_exceptions[DARLING_SELECT_FD_SETSIZE / 32] = { 0 };
	int inspect_exceptions = efds != NULL && nfds > 0 &&
		nfds <= DARLING_SELECT_FD_SETSIZE;
	int pending_pty_count = 0;

	if (inspect_exceptions) {
		memcpy(requested_exceptions, efds, (size_t)((nfds + 31) / 32) * sizeof(uint32_t));
		pending_pty_count = add_pty_hangups_to_exception_set(nfds,
				requested_exceptions, pending_pty_exceptions);
	}

	if (timeout != NULL)
	{
		lts.tv_sec = timeout->tv_sec;
		lts.tv_nsec = timeout->tv_nsec;
	}
	if (pending_pty_count > 0) {
		lts.tv_sec = 0;
		lts.tv_nsec = 0;
	}
	if (mask != NULL)
	{
		sigset_bsd_to_linux(mask, &lmask);

		data[0] = (long)&lmask;
		data[1] = sizeof(lmask);
	}

	ret = LINUX_SYSCALL(__NR_pselect6, nfds, rfds, wfds, efds,
			(timeout != NULL || pending_pty_count > 0) ? &lts : NULL,
			(mask != NULL) ? data : NULL);

	if (ret >= 0 && inspect_exceptions)
		ret += add_pty_hangups_to_exception_set(nfds, requested_exceptions, efds);
	else if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}
