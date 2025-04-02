#include "bsdthread_create.h"
#include "bsdthread_register.h"
#include "../base.h"
#include "../errno.h"
#include <linux-syscalls/linux.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/errno.h>
#include "../mman/mman.h"
#include "../simple.h"
#include "../elfcalls_wrapper.h"
#include "../machdep/tls.h"
#include "../mach/mach_traps.h"

#include <darlingserver/rpc.h>
#include "../guarded/table.h"

extern void *memset(void *s, int c, size_t n);

#define PTHREAD_START_CUSTOM    0x01000000
#define PTHREAD_START_SETSCHED  0x02000000
#define PTHREAD_START_DETACHED  0x04000000
#define PTHREAD_START_POLICY_BITSHIFT 16
#define PTHREAD_START_POLICY_MASK 0xff
#define PTHREAD_START_IMPORTANCE_MASK 0xffff

#define STACK_GUARD_SIZE 4096

static bool _uses_threads = false;

// http://www.tldp.org/FAQ/Threads-FAQ/clone.c

static void rpc_guard(int fd) {
	guard_entry_options_t options;
	options.close = __dserver_close_socket;
	guard_table_add(fd, guard_flag_prevent_close | guard_flag_close_on_fork, &options);
};

static void rpc_unguard(int fd) {
	guard_table_remove(fd);
};

static const struct darling_thread_create_callbacks callbacks = {
	.thread_self_trap = &thread_self_trap_impl,
	.thread_set_tsd_base = &sys_thread_set_tsd_base,
	.rpc_guard = rpc_guard,
	.rpc_unguard = rpc_unguard,
};

long sys_bsdthread_create(void* thread_start, void* arg,
		void* stack, void* pthread, uint32_t flags)
{
#ifndef VARIANT_DYLD // dyld doesn't create threads
	int ret;
	unsigned long stacksize = 0;

	_uses_threads = true;

#ifndef BSDTHREAD_WRAP_LINUX_PTHREAD
	if (!(flags & PTHREAD_START_CUSTOM))
	{
		// We have to allocate stack ourselves
		pthread = stack = thread_stack_allocate(stacksize);
	}

	ret = darling_thread_create((void**) stack, pthread_entry_point_wrapper, thread_start,
			arg, stacksize, flags);
#else
	return __darling_thread_create(((uintptr_t)stack), pthread_obj_size,
			pthread_entry_point_wrapper, thread_start, arg, (uintptr_t) stack, flags,
			&callbacks, pthread);
#endif

	if (ret < 0)
		return errno_linux_to_bsd(ret);

	return pthread;
#else
	return -ENOSYS;
#endif
}

void* thread_stack_allocate(unsigned long stacksize)
{
	unsigned long allocsize;
	void* stack;

	// The pthread object is placed above stack area
	allocsize = stacksize + pthread_obj_size + STACK_GUARD_SIZE;
	allocsize = (allocsize + 4095) & ~4095; // round up to page size

	stack = (void**) sys_mmap(NULL, allocsize, PROT_READ | PROT_WRITE,
			MAP_ANON | MAP_PRIVATE, -1, 0);

	// Protect the stack guard
	sys_mprotect(stack, STACK_GUARD_SIZE, PROT_NONE);
	
	if (((intptr_t)stack) < 0 && ((intptr_t)stack) >= -4095)
		return NULL;

	return (void*) (((uintptr_t)stack) + stacksize + STACK_GUARD_SIZE);
}

#if 0
int darling_thread_create(void** stack, void* entry_point, uintptr_t arg3,
		uintptr_t arg4, uintptr_t arg5, uintptr_t arg6)
{
	int ret;

#if defined(__x86_64__)
	// Store these arguments to pthread_entry point on the stack
	stack[-1] = entry_point;
	stack[-2] = arg3;
	stack[-3] = arg4;
	stack[-4] = arg5;
	stack[-5] = arg6;

	__asm__ __volatile__ (
			"syscall\n" // invoke sys_clone
			"testl %0, %0\n" // if in parent thread, jump away
			"jne 1f\n"
			"subq $40, %%rsp\n" // protect what we have stored on stack
			"call _thread_self_trap\n" // get thread_self Mach port
			"addq $40, %%rsp\n"
			"movl %%eax, %%esi\n" // thread_self is 2nd arg to pthread_entry_point
			"movq %%rsp, %%rdi\n" // pthread_self as 1st arg
			"movq -16(%%rsp), %%rdx\n" // thread_start as 3rd arg
			"movq -32(%%rsp), %%r8\n" // stack_size as 5th arg
			"movq -24(%%rsp), %%rcx\n" // thread arg as 4th arg
			"movq -40(%%rsp), %%r9\n" // flags as 6th arg
			"movq -8(%%rsp), %%rbx\n"
			"jmp *%%rbx\n"
			"1:\n"
			: "=a"(ret)
			: "0" (__NR_clone),
			"D"(LINUX_CLONE_THREAD | LINUX_CLONE_VM | LINUX_CLONE_SIGHAND 
	            | LINUX_CLONE_FILES | LINUX_CLONE_FS | LINUX_CLONE_SYSVSEM),
			"S"(stack));
#elif defined(__i386__)
#	warning Missing clone call assembly!
#else
#	warning Missing clone call assembly!
#endif
	return ret;
}
#endif

bool uses_threads(void)
{
	return _uses_threads;
}

