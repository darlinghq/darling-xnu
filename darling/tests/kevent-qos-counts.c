/* Host test for sys_kevent_qos: caller-controlled counts must not size stack allocations. */
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Skip the Darwin-only headers the unit includes; provide just what it uses. */
#define LINUX_KEVENT_QOS_H
#define LINUX_BASE_H
#define LINUX_ERRNO_H
#define LINUX_DEBUG_H
#define LINUX_FCNTL_H
#define _ASM_GENERAL_UNISTD_H
#define LINUX_KQUEUE_H
#define WORKQ_KERNRETURN_H

#define KEVENT_FLAG_IMMEDIATE 0x000001
#define KEVENT_FLAG_WORKQ 0x000020
#define WQOPS_QUEUE_REQTHREAD_FOR_KEVENT 0x100020
#define F_SETFD 2
#define FD_CLOEXEC 1

struct kevent64_s {
	uint64_t ident;
	int16_t filter;
	uint16_t flags;
	uint32_t fflags;
	int64_t data;
	uint64_t udata;
	uint64_t ext[2];
};

struct kevent_qos_s {
	uint64_t ident;
	int16_t filter;
	uint16_t flags;
	int32_t qos;
	uint64_t udata;
	uint32_t fflags;
	uint32_t xflags;
	int64_t data;
	uint64_t ext[4];
};

struct wq_kevent_data {
	struct kevent_qos_s *events;
	int nevents;
	int sem;
};

long sys_kevent_qos(int kq, const struct kevent_qos_s *changelist, int nchanges,
		struct kevent_qos_s *eventlist, int nevents,
		void *data_out, unsigned long *data_available, unsigned int flags);
long sys_kqueue(void);
long sys_fcntl(int fd, int cmd, long arg);
long sys_workq_kernreturn(int options, void *item, int affinity, int prio);
int sem_down(int *sem, int timeout);

#include "../src/libsystem_kernel/emulation/src/xnu_syscall/bsd/impl/kqueue/kevent_qos.c"

static int last_nchanges = -1, last_nevents = -1;

long sys_kevent64(int kq, const struct kevent64_s *changelist, int nchanges,
		struct kevent64_s *eventlist, int nevents, unsigned int flags,
		const struct timespec *timeout)
{
	(void) kq; (void) flags; (void) timeout;
	last_nchanges = nchanges;
	last_nevents = nevents;
	/* Read every change and write every event slot so ASan sees any undersized buffer. */
	volatile uint64_t sum = 0;
	for (int i = 0; i < nchanges; i++)
		sum += changelist[i].ident;
	for (int i = 0; i < nevents; i++) {
		memset(&eventlist[i], 0, sizeof(eventlist[i]));
		eventlist[i].ident = 1000 + i;
	}
	return nevents;
}

long sys_kqueue(void) { return 3; }
long sys_fcntl(int fd, int cmd, long arg) { (void) fd; (void) cmd; (void) arg; return 0; }
long sys_workq_kernreturn(int options, void *item, int affinity, int prio)
{
	(void) options; (void) item; (void) affinity; (void) prio;
	return 0;
}
int sem_down(int *sem, int timeout) { (void) sem; (void) timeout; return 0; }

static int failures;

#define CHECK(cond, ...) do { if (!(cond)) { failures++; fprintf(stderr, __VA_ARGS__); fputc('\n', stderr); } } while (0)

int main(void)
{
	static const int counts[] = { 0, 1, 16, 17, 100000 };

	for (unsigned c = 0; c < sizeof(counts) / sizeof(counts[0]); c++) {
		int n = counts[c];
		struct kevent_qos_s *changes = calloc(n ? n : 1, sizeof(*changes));
		struct kevent_qos_s *events = calloc(n ? n : 1, sizeof(*events));
		for (int i = 0; i < n; i++)
			changes[i].ident = i;

		long rv = sys_kevent_qos(5, changes, n, events, n, NULL, NULL, 0);
		CHECK(rv == n, "n=%d plain: rv=%ld", n, rv);
		CHECK(last_nchanges == n && last_nevents == n, "n=%d plain: forwarded %d/%d", n, last_nchanges, last_nevents);
		for (int i = 0; i < n; i++)
			if (events[i].ident != (uint64_t) (1000 + i)) {
				CHECK(0, "n=%d plain: event %d not copied back", n, i);
				break;
			}
		free(changes);
		free(events);
	}

	/* WORKQ with no event room: one internal slot is used, caller's list untouched. */
	{
		struct kevent_qos_s ev;
		memset(&ev, 0, sizeof(ev));
		long rv = sys_kevent_qos(-1, NULL, 0, &ev, 0, NULL, NULL, KEVENT_FLAG_WORKQ);
		CHECK(rv == 1, "workq nevents=0: rv=%ld", rv);
		CHECK(last_nevents == 1, "workq nevents=0: forwarded %d", last_nevents);
	}

	CHECK(sys_kevent_qos(5, NULL, -1, NULL, 0, NULL, NULL, 0) == -EINVAL, "negative nchanges not EINVAL");
	CHECK(sys_kevent_qos(5, NULL, 0, NULL, -1, NULL, NULL, 0) == -EINVAL, "negative nevents not EINVAL");

	if (failures)
		return 1;
	puts("ok");
	return 0;
}
