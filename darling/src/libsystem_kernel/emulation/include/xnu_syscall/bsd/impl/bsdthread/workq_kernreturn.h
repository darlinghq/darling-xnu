#ifndef WORKQ_KERNRETURN_H
#define WORKQ_KERNRETURN_H

#define PRIVATE 1
#include <sys/event.h>

// Darling specific
#define WQOPS_QUEUE_REQTHREAD_FOR_KEVENT  0x100020

// A kevent delivery handed to a workqueue thread. It lives on the stack of
// the thread in kevent_qos() that waits on `sem` until the worker returns.
struct wq_kevent_data
{
	struct kevent_qos_s* events; // events in; returned events out
	int nevents;                 // number of events in; returned events out
	int capacity;                // room in `events` for returned events
	int sem;
	// owned by the workqueue code while the request is queued:
	int flags;
	struct wq_kevent_data* next;
};

struct timespec;

long sys_workq_kernreturn(int options, void* item, int affinity, int prio);

void workq_postfork_child(void);

int sem_down(int* sem, int timeout);
void sem_up(int* sem);

#endif // WORKQ_KERNRETURN_H
