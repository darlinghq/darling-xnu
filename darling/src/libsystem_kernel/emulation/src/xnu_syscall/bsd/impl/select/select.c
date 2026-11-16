#include <darling/emulation/xnu_syscall/bsd/impl/select/select.h>

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/xnu_syscall/bsd/helper/bsdthread/cancelable.h>
#include <darling/emulation/xnu_syscall/bsd/impl/select/pselect.h>
#include <darling/emulation/common/sigsafe_lock.h>
#include <libsimple/lock.h>

#define LINUX_EBADF 9
#define LINUX_SOL_TCP 6
#define LINUX_TCP_INFO 11
#define LINUX_TCP_CLOSE 7
#define LINUX_SYS_GETSOCKOPT 15

// A TCP socket in Linux's TCP_CLOSE state with no segments in either direction was never connected. Linux reports
// it as both readable and writable, but XNU's soreadable()/sowriteable() do not: an unconnected socket is not a
// wake-up source until it is closed. Keep such sockets out of select()'s result and, when there is nothing else to
// report, wait for close() with an eventfd and return EBADF, as Darwin does.
#define SELECT_FILTER_MAX_FDS 1024 // Darwin's FD_SETSIZE; larger sets are left alone

#define LINUX_EFD_CLOEXEC 02000000
#define LINUX_EFD_NONBLOCK 04000
#define SELECT_WATCHES 32
#define SELECT_WATCH_FDS 8

// The beginning of Linux's struct tcp_info, through tcpi_segs_in (Linux 4.2).
struct select_tcp_info {
	unsigned char state;
	unsigned char other[135];
	unsigned int segs_out;
	unsigned int segs_in;
};

struct select_linux_timespec {
	long tv_sec;
	long tv_nsec;
};

static long select_getsockopt(int fd, int level, int name, void* value, unsigned int* len)
{
#ifdef __NR_socketcall
	return LINUX_SYSCALL(__NR_socketcall, LINUX_SYS_GETSOCKOPT,
			((long[6]) { fd, level, name, (long)value, (long)len, 0 }));
#else
	return LINUX_SYSCALL(__NR_getsockopt, fd, level, name, value, len);
#endif
}

static int tcp_never_connected(int fd)
{
	struct select_tcp_info info;
	unsigned int len = sizeof(info);

	memset(&info, 0, sizeof(info));
	if (select_getsockopt(fd, LINUX_SOL_TCP, LINUX_TCP_INFO, &info, &len) < 0 || len < sizeof(info))
		return 0;
	return info.state == LINUX_TCP_CLOSE && info.segs_out == 0 && info.segs_in == 0;
}

// The waits watching never-connected sockets for close(): the sockets, and the eventfd close() writes to.
static struct select_watch {
	int wake; // -1: free
	int fds[SELECT_WATCH_FDS];
	int count;
} select_watches[SELECT_WATCHES] = { [0 ... SELECT_WATCHES - 1] = { .wake = -1 } };
static libsimple_lock_t select_watches_lock = LIBSIMPLE_LOCK_INITIALIZER;
static volatile int select_watch_count;

void select_fd_closed(int fd)
{
	if (select_watch_count == 0)
		return;
	unsigned long long mask = sigsafe_lock(&select_watches_lock);
	for (int i = 0; i < SELECT_WATCHES; i++) {
		struct select_watch* w = &select_watches[i];
		if (w->wake < 0)
			continue;
		for (int j = 0; j < w->count; j++) {
			if (w->fds[j] == fd) {
				unsigned long long one = 1;
				LINUX_SYSCALL(__NR_write, w->wake, &one, sizeof(one));
				break;
			}
		}
	}
	sigsafe_unlock(&select_watches_lock, mask);
}

// A watch over the never-connected sockets of the sets asked for (fds[], count): its slot, or -1 (none free, or no
// eventfd: the wait goes on unwatched).
static int watch_start(const int* fds, int count)
{
	int wake = LINUX_SYSCALL(__NR_eventfd2, 0, LINUX_EFD_CLOEXEC | LINUX_EFD_NONBLOCK);
	if (wake < 0)
		return -1;
	unsigned long long mask = sigsafe_lock(&select_watches_lock);
	for (int i = 0; i < SELECT_WATCHES; i++) {
		struct select_watch* w = &select_watches[i];
		if (w->wake >= 0)
			continue;
		w->wake = wake;
		w->count = count;
		for (int j = 0; j < count; j++)
			w->fds[j] = fds[j];
		select_watch_count++;
		sigsafe_unlock(&select_watches_lock, mask);
		return i;
	}
	sigsafe_unlock(&select_watches_lock, mask);
	LINUX_SYSCALL1(__NR_close, wake);
	return -1;
}

static void watch_end(int slot)
{
	unsigned long long mask = sigsafe_lock(&select_watches_lock);
	int wake = select_watches[slot].wake;
	select_watches[slot].wake = -1;
	select_watch_count--;
	sigsafe_unlock(&select_watches_lock, mask);
	LINUX_SYSCALL1(__NR_close, wake);
}

static int fd_isset(const void* set, int fd) { return set && (((const unsigned char*) set)[fd / 8] >> (fd % 8)) & 1; }
static void fd_clr(void* set, int fd) { if (set) ((unsigned char*) set)[fd / 8] &= ~(1 << (fd % 8)); }

// Takes the never-connected TCP sockets out of a select() result of ret bits; returns the bits left. Those sockets
// are also taken out of the sets asked for (in_*), for the next wait, and added to removed[] (*nremoved).
static int filter_never_connected(int nfds, void* rfds, void* wfds, void* in_r, void* in_w, int ret, int* removed,
		int* nremoved)
{
	for (int fd = 0; fd < nfds && ret > 0; fd++) {
		int r = fd_isset(rfds, fd), w = fd_isset(wfds, fd);
		if (!r && !w)
			continue;
		if (!tcp_never_connected(fd))
			continue;
		fd_clr(rfds, fd);
		fd_clr(wfds, fd);
		fd_clr(in_r, fd);
		fd_clr(in_w, fd);
		ret -= r + w;
		if (*nremoved < SELECT_WATCH_FDS)
			removed[(*nremoved)++] = fd;
	}
	return ret;
}

static long long select_saturating_add(long long a, long long b)
{
	const long long max = 9223372036854775807LL;
	if (b > 0 && a > max - b)
		return max;
	if (b < 0 && a < -max - b)
		return -max;
	return a + b;
}

static long long select_seconds_to_ns(long long seconds)
{
	const long long max = 9223372036854775807LL;
	if (seconds > max / 1000000000LL)
		return max;
	if (seconds < -max / 1000000000LL)
		return -max;
	return seconds * 1000000000LL;
}

static long long select_monotonic_ns(void)
{
#ifdef __NR_clock_gettime
	struct select_linux_timespec ts;
	if (LINUX_SYSCALL(__NR_clock_gettime, 1 /* CLOCK_MONOTONIC */, &ts) < 0)
		return -1;
	return select_saturating_add((long long)ts.tv_sec * 1000000000LL, ts.tv_nsec);
#else
	return -1;
#endif
}

static long sys_select_common(int nfds, void* rfds, void* wfds, void* efds, struct bsd_timeval* timeout, bool cancelable)
{
	// Go through pselect, which reports the exceptional conditions of pseudo-terminal masters the way macOS does;
	// pselect() takes a timespec, not a timeval.
	struct bsd_timespec ts;
	if (timeout != NULL)
	{
		ts.tv_sec = timeout->tv_sec;
		ts.tv_nsec = (long)timeout->tv_usec * 1000L;
	}

	// The kernel advances its copy of the timeout while waiting; keep our own deadline for repeated waits.
	long long start = timeout != NULL ? select_monotonic_ns() : 0;
	bool deadline_valid = timeout != NULL && start >= 0;
	long long deadline = 0;
	if (deadline_valid)
	{
		deadline = select_saturating_add(start,
				select_saturating_add(select_seconds_to_ns(timeout->tv_sec),
					(long long)timeout->tv_usec * 1000LL));
	}

	// The sets asked for, to wait again with. The kernel's fd_set uses unsigned long words.
	unsigned char in_r[SELECT_FILTER_MAX_FDS / 8], in_w[SELECT_FILTER_MAX_FDS / 8], in_e[SELECT_FILTER_MAX_FDS / 8];
	unsigned char empty_r[SELECT_FILTER_MAX_FDS / 8];
	int filter = nfds > 0 && nfds <= SELECT_FILTER_MAX_FDS && (rfds || wfds);
	unsigned long set_bytes = ((nfds + 8 * sizeof(unsigned long) - 1) / (8 * sizeof(unsigned long))) *
		sizeof(unsigned long);
	int removed[SELECT_WATCH_FDS], nremoved = 0, watch = -1, wait_nfds = nfds;
	int ret;

	if (filter)
	{
		if (rfds) memcpy(in_r, rfds, set_bytes);
		if (wfds) memcpy(in_w, wfds, set_bytes);
		if (efds) memcpy(in_e, efds, set_bytes);
	}
	for (;;)
	{
		ret = sys_pselect_common(wait_nfds, rfds, wfds, efds,
				(timeout != NULL) ? &ts : NULL, NULL, cancelable);
		if (ret > 0 && watch >= 0)
		{
			// A watched socket was closed: Darwin's select() fails with EBADF.
			int wake = select_watches[watch].wake;
			void* r = rfds ? rfds : empty_r;
			if (fd_isset(r, wake))
			{
				ret = errno_linux_to_bsd(-LINUX_EBADF);
				break;
			}
		}
		if (ret <= 0 || !filter)
			break;

		int left = filter_never_connected(nfds, rfds, wfds, rfds ? in_r : NULL, wfds ? in_w : NULL, ret,
				removed, &nremoved);
		if (left > 0 || (timeout != NULL && ts.tv_sec == 0 && ts.tv_nsec == 0))
		{
			ret = left;
			break;
		}

		// Nothing else was ready: wait again for the rest of the time, without those sockets but woken by
		// close().
		if (rfds) memcpy(rfds, in_r, set_bytes);
		if (wfds) memcpy(wfds, in_w, set_bytes);
		if (efds) memcpy(efds, in_e, set_bytes);
		if (watch < 0 && nremoved > 0 && (watch = watch_start(removed, nremoved)) >= 0)
		{
			int wake = select_watches[watch].wake;
			if (wake >= SELECT_FILTER_MAX_FDS)
			{
				watch_end(watch);
				watch = -1;
			}
			else
			{
				if (!rfds)
				{
					memset(empty_r, 0, sizeof(empty_r));
					rfds = empty_r;
				}
				wait_nfds = wake + 1 > nfds ? wake + 1 : nfds;
			}
		}
		if (watch >= 0)
		{
			unsigned char* r = rfds;
			int wake = select_watches[watch].wake;
			r[wake / 8] |= 1 << (wake % 8);
		}
		if (deadline_valid)
		{
			long long now = select_monotonic_ns();
			long long remaining = now < 0 ? 0 : deadline - now;
			if (remaining < 0)
				remaining = 0;
			ts.tv_sec = (long)(remaining / 1000000000LL);
			ts.tv_nsec = (long)(remaining % 1000000000LL);
		}
	}
	if (watch >= 0)
	{
		// The eventfd is no bit of the caller's.
		if (rfds && ret >= 0)
		{
			int wake = select_watches[watch].wake;
			((unsigned char*) rfds)[wake / 8] &= ~(1 << (wake % 8));
		}
		if (rfds == empty_r)
			rfds = NULL;
		watch_end(watch);
	}

	return ret;
}

long sys_select(int nfds, void* rfds, void* wfds, void* efds, struct bsd_timeval* timeout)
{
	CANCELATION_POINT();
	return sys_select_common(nfds, rfds, wfds, efds, timeout, true);
}

long sys_select_nocancel(int nfds, void* rfds, void* wfds, void* efds, struct bsd_timeval* timeout)
{
	return sys_select_common(nfds, rfds, wfds, efds, timeout, false);
}
