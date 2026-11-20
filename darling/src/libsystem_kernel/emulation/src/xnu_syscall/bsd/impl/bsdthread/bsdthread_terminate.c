#include <darling/emulation/xnu_syscall/bsd/impl/bsdthread/bsdthread_terminate.h>

#include <sys/errno.h>
#include <stddef.h>
#include <stdint.h>

#include <darling/emulation/xnu_syscall/bsd/impl/bsdthread/bsdthread_register.h>
#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/linux_premigration/elfcalls_wrapper.h>
#include <darling/emulation/common/guarded/table.h>
#include <darling/emulation/other/mach/lkm.h>
#include <darling/emulation/xnu_syscall/bsd/impl/psynch/psynch_inproc.h>

int bsdthread_terminate_trap(
                uintptr_t stackaddr,
                unsigned long freesize,
                int thread,
                int sem);
int semaphore_signal_trap_impl(int signal_name);

extern void _xtrace_thread_exit(void);

long sys_bsdthread_terminate(void* stackaddr, unsigned long freesize, int port,
		int join_sem)
{
#ifndef VARIANT_DYLD
#ifdef BSDTHREAD_WRAP_LINUX_PTHREAD
	// Implemented in libdyld
	extern int __darling_thread_terminate(void* stackaddr,
			unsigned long freesize, unsigned long pthobj_size);

	// the in-process psynch state of this thread can go to the next thread;
	// before the joiner is woken, as it may free our TSD
	psynch_inproc_thread_exit();

	semaphore_signal_trap_impl(join_sem);

	// point of no return; let xtrace know
	_xtrace_thread_exit();

	// we can also unguard the RPC FD for this thread now
	guard_table_remove(mach_driver_get_fd());

	return __darling_thread_terminate(stackaddr, freesize, pthread_obj_size);
#else
	return bsdthread_terminate_trap((uintptr_t) stackaddr, freesize,
			port, join_sem);
#endif
#else
	return -ENOSYS;
#endif
}
