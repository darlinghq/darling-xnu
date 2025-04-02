#ifndef DARLING_BSD_EMULATION_WORKQ_KERNRETURN_H
#define DARLING_BSD_EMULATION_WORKQ_KERNRETURN_H

#include <emulation/conversion/variables/bsd/sys/event.h>

// Darling specific
#define WQOPS_QUEUE_REQTHREAD_FOR_KEVENT  0x100020

struct wq_kevent_data
{
	struct bsd_kevent_qos_s* events;
	int nevents;
	int sem;
};

long sys_workq_kernreturn(int options, void* item, int affinity, int prio);

int sem_down(int* sem, int timeout);
void sem_up(int* sem);

#endif

