#include <darling/emulation/xnu_syscall/bsd/impl/kqueue/kevent_qos.h>

#include <stddef.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/mman.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/common/simple.h>
#include <darling/emulation/xnu_syscall/bsd/impl/fcntl/fcntl.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/xnu_syscall/bsd/impl/kqueue/kqueue.h>
#include <darling/emulation/xnu_syscall/bsd/impl/bsdthread/workq_kernreturn.h>
#include <darling/emulation/xnu_syscall/bsd/impl/kqueue/kevent64.h>
#include <darling/emulation/xnu_syscall/bsd/impl/mman/mman.h>

static int default_kq = -1;

extern void* memmove(void* dst, const void* src, __SIZE_TYPE__ n);

static void kevent_qos_to_64(const struct kevent_qos_s *ev, struct kevent64_s* ev64);
static void kevent_64_to_qos(const struct kevent64_s* ev64, struct kevent_qos_s *ev);

#define KEVENT_STACK_BATCH 16

// Batches too large for the stack are mapped directly, as libsystem_kernel
// sits below the malloc implementation. Returns NULL on failure.
static struct kevent64_s* kevent_buffer_map(int count, unsigned long* size)
{
	void* buffer;

	if ((unsigned long) count > ~0ul / sizeof(struct kevent64_s))
		return NULL;

	*size = (unsigned long) count * sizeof(struct kevent64_s);
	buffer = sys_mmap(NULL, *size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
	if ((unsigned long) buffer > -4096ul)
		return NULL;

	return buffer;
}

static void kevent_buffer_unmap(struct kevent64_s* buffer, unsigned long size)
{
	if (buffer != NULL)
		sys_munmap(buffer, size);
}

long sys_kevent_qos(int	kq, const struct kevent_qos_s *changelist, int nchanges,
			struct	kevent_qos_s *eventlist, int nevents,
			void* data_out, unsigned long* data_available, unsigned int flags)
{
	int rv;
	struct kevent64_s* changelist64 = NULL;
	struct kevent64_s* eventlist64 = NULL;
	struct wq_kevent_data wq_kevent;
	struct timespec polling_timeout = {};
	struct kevent64_s small_changes[KEVENT_STACK_BATCH], small_events[KEVENT_STACK_BATCH];
	struct kevent_qos_s workq_event;
	struct kevent64_s* mapped_changes = NULL;
	struct kevent64_s* mapped_events = NULL;
	unsigned long mapped_changes_size = 0, mapped_events_size = 0;

	if (nchanges < 0 || nevents < 0)
		return -EINVAL;

	if ((kq == -1) != !!(flags & KEVENT_FLAG_WORKQ))
		return -EINVAL;

	if (default_kq == -1)
	{
		default_kq = sys_kqueue();
		sys_fcntl_nocancel(default_kq, F_SETFD, FD_CLOEXEC);
	}

	// Counts come from the caller: keep small batches on the stack, map larger ones
	if (changelist != NULL && nchanges > 0)
	{
		int i;

		if (nchanges <= KEVENT_STACK_BATCH)
			changelist64 = small_changes;
		else
		{
			mapped_changes = kevent_buffer_map(nchanges, &mapped_changes_size);
			if (mapped_changes == NULL)
				return -ENOMEM;
			changelist64 = mapped_changes;
		}
		for (i = 0; i < nchanges; i++)
			kevent_qos_to_64(&changelist[i], &changelist64[i]);
	}
	if (eventlist != NULL && nevents > 0)
	{
		if (nevents <= KEVENT_STACK_BATCH)
			eventlist64 = small_events;
		else
		{
			mapped_events = kevent_buffer_map(nevents, &mapped_events_size);
			if (mapped_events == NULL)
			{
				kevent_buffer_unmap(mapped_changes, mapped_changes_size);
				return -ENOMEM;
			}
			eventlist64 = mapped_events;
		}
	}
	if (nevents == 0 && (flags & KEVENT_FLAG_WORKQ))
	{
		eventlist = &workq_event;
		nevents = 1;
		eventlist64 = small_events;
	}

	rv = sys_kevent64(kq, changelist64, nchanges, eventlist64, nevents, flags, (flags & KEVENT_FLAG_IMMEDIATE) ? &polling_timeout : NULL);

	if (rv > 0)
	{
		int i;

		for (i = 0; i < rv; i++)
			kevent_64_to_qos(&eventlist64[i], &eventlist[i]);
	}

	if ((flags & KEVENT_FLAG_WORKQ) && rv >= 0)
	{
		// Pass to workqueue and wait
		wq_kevent.sem = 0;
		wq_kevent.events = eventlist;
		wq_kevent.nevents = rv;
		wq_kevent.capacity = nevents;

		sys_workq_kernreturn(WQOPS_QUEUE_REQTHREAD_FOR_KEVENT, &wq_kevent, 1, 0);
		
		// __simple_printf("waiting for workq processing\n");
		sem_down(&wq_kevent.sem, -1);

		rv = wq_kevent.nevents;
		memmove(eventlist, wq_kevent.events, rv * sizeof(struct kevent_qos_s));
		// __simple_printf("workq done processing, returning %d to caller\n", rv);
	}

	kevent_buffer_unmap(mapped_events, mapped_events_size);
	kevent_buffer_unmap(mapped_changes, mapped_changes_size);
	return rv;
}


void kevent_qos_to_64(const struct kevent_qos_s *ev, struct kevent64_s* ev64)
{
	ev64->ident = ev->ident;
	ev64->filter = ev->filter;
	ev64->flags = ev->flags;
	ev64->fflags = ev->fflags;
	ev64->data = ev->data;
	ev64->udata = ev->udata;
	ev64->ext[0] = ev->ext[0];
	ev64->ext[1] = ev->ext[1];
}

void kevent_64_to_qos(const struct kevent64_s* ev64, struct kevent_qos_s *ev)
{
	ev->ident = ev64->ident;
	ev->filter = ev64->filter;
	ev->flags = ev64->flags;
	ev->qos = 0;
	ev->fflags = ev64->fflags;
	ev->data = ev64->data;
	ev->udata = ev64->udata;
	ev->ext[0] = ev64->ext[0];
	ev->ext[1] = ev64->ext[1];
	ev->ext[2] = ev->ext[3] = 0;
}
