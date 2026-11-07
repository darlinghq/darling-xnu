#include <darling/emulation/xnu_syscall/bsd/impl/bsdthread/pthread_canceled.h>

#include <stddef.h>
#include <sys/errno.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/common/tsd.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/conversion/duct_errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/xnu_syscall/bsd/impl/bsdthread/bsdthread_register.h>

#include <darlingserver/rpc.h>

// The TSD slot __PTK_DARLING_CANCEL_MARKED is shared with darlingserver (see set_thread_handles()).
// darlingserver sets its first byte when it marks the thread for cancellation.
// The thread sets its second byte while it blocks in a cancelable syscall, which tells darlingserver
// to interrupt the syscall (with SIGNAL_SIGEXC_SUSPEND) when it marks the thread.
#define CANCEL_MARKED_BYTE  0
#define CANCEL_WAITING_BYTE 1

static volatile uint8_t* cancel_slot(void)
{
	// before libpthread has registered (e.g. in dyld), there may be no TSD to read
	if (!bsdthread_registered())
		return NULL;
	return (volatile uint8_t*) bsdthread_cancel_marked_slot();
}

static bool cancel_marked(void)
{
	volatile uint8_t* slot = cancel_slot();
	return slot != NULL && __atomic_load_n(&slot[CANCEL_MARKED_BYTE], __ATOMIC_RELAXED) != 0;
}

long sys_pthread_canceled(int action)
{
	if (action == 0 && !cancel_marked())
		return -EINVAL;

	int ret = dserver_rpc_pthread_canceled(action);
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);
	else if (action == 0)
	{
		// the mark has been acted on and darlingserver won't mark this thread again,
		// so the cancellation points in its cleanup handlers can skip the call
		__atomic_store_n(&cancel_slot()[CANCEL_MARKED_BYTE], 0, __ATOMIC_RELAXED);
	}

	return ret;
}

// unlike sys_pthread_canceled(0), this leaves the thread marked,
// so that the EINTR handling in libc can still act on it
bool pthread_testcancel_presyscall(void)
{
	return cancel_marked() && dserver_rpc_pthread_testcancel() == 0;
}

// linux_syscall_cancelable() is linux_syscall() with an extra argument, the address of the marked byte.
// It checks the byte, then makes the syscall. The signal handler moves a thread that it interrupts
// between the check and the syscall instruction to an exit that returns -LINUX_EINTR, if the thread
// has been marked. That includes a thread blocked in the syscall: our handlers use SA_RESTART, so for
// a syscall that the kernel restarts (e.g. a futex wait without a timeout), it points the thread back
// at the syscall instruction before the handler runs. A signal that arrives before the check leaves
// it to the check to see the mark.

#define CANCELABLE_STR(x) #x
#define CANCELABLE_XSTR(x) CANCELABLE_STR(x)

long linux_syscall_cancelable(long a1, long a2, long a3, long a4, long a5, long a6, int nr, volatile uint8_t* marked);

// the window from the check up to the syscall instruction ([0], [1]), and the exit ([2])
extern const uintptr_t linux_syscall_cancelable_window[3];

#if defined(__x86_64__)

__asm__(
	".text\n"
	".p2align 4\n"
	".globl _linux_syscall_cancelable\n"
	".private_extern _linux_syscall_cancelable\n"
	"_linux_syscall_cancelable:\n"
	"	movq 16(%rsp), %rax\n"
	"Lcancelable_start:\n"
	"	cmpb $0, (%rax)\n"
	"	jne Lcancelable_exit\n"
	"	movl 8(%rsp), %eax\n"
	"	movq %rcx, %r10\n"
	"	syscall\n"
	"Lcancelable_end:\n"
	"	ret\n"
	"Lcancelable_exit:\n"
	"	movq $-" CANCELABLE_XSTR(LINUX_EINTR) ", %rax\n"
	"	ret\n"
	"\n"
	".section __DATA,__const\n"
	".p2align 3\n"
	".globl _linux_syscall_cancelable_window\n"
	".private_extern _linux_syscall_cancelable_window\n"
	"_linux_syscall_cancelable_window:\n"
	"	.quad Lcancelable_start\n"
	"	.quad Lcancelable_end\n"
	"	.quad Lcancelable_exit\n"
	".text\n"
);

#elif defined(__i386__)

// int $0x80, as in linux_syscall(): with 32in64, there is no 32-bit VDSO for sysenter
__asm__(
	".text\n"
	".p2align 4\n"
	".globl _linux_syscall_cancelable\n"
	".private_extern _linux_syscall_cancelable\n"
	"_linux_syscall_cancelable:\n"
	"	push %ebx\n"
	"	push %esi\n"
	"	push %edi\n"
	"	push %ebp\n"
	"	mov 48(%esp), %eax\n"
	"Lcancelable_start:\n"
	"	cmpb $0, (%eax)\n"
	"	jne Lcancelable_exit\n"
	"	mov 20(%esp), %ebx\n"
	"	mov 24(%esp), %ecx\n"
	"	mov 28(%esp), %edx\n"
	"	mov 32(%esp), %esi\n"
	"	mov 36(%esp), %edi\n"
	"	mov 40(%esp), %ebp\n"
	"	mov 44(%esp), %eax\n"
	"	int $0x80\n"
	"Lcancelable_end:\n"
	"	pop %ebp\n"
	"	pop %edi\n"
	"	pop %esi\n"
	"	pop %ebx\n"
	"	ret\n"
	"Lcancelable_exit:\n"
	"	mov $-" CANCELABLE_XSTR(LINUX_EINTR) ", %eax\n"
	"	jmp Lcancelable_end\n"
	"\n"
	".section __DATA,__const\n"
	".p2align 2\n"
	".globl _linux_syscall_cancelable_window\n"
	".private_extern _linux_syscall_cancelable_window\n"
	"_linux_syscall_cancelable_window:\n"
	"	.long Lcancelable_start\n"
	"	.long Lcancelable_end\n"
	"	.long Lcancelable_exit\n"
	".text\n"
);

#elif defined(__aarch64__) || defined(__arm64__)

__asm__(
	".text\n"
	".p2align 2\n"
	".globl _linux_syscall_cancelable\n"
	".private_extern _linux_syscall_cancelable\n"
	"_linux_syscall_cancelable:\n"
	"Lcancelable_start:\n"
	"	ldrb w9, [x7]\n"
	"	cbnz w9, Lcancelable_exit\n"
	"	mov x8, x6\n"
	"	svc #0\n"
	"Lcancelable_end:\n"
	"	ret\n"
	"Lcancelable_exit:\n"
	"	mov x0, #-" CANCELABLE_XSTR(LINUX_EINTR) "\n"
	"	ret\n"
	"\n"
	".section __DATA,__const\n"
	".p2align 3\n"
	".globl _linux_syscall_cancelable_window\n"
	".private_extern _linux_syscall_cancelable_window\n"
	"_linux_syscall_cancelable_window:\n"
	"	.quad Lcancelable_start\n"
	"	.quad Lcancelable_end\n"
	"	.quad Lcancelable_exit\n"
	".text\n"
);

#else
#	error Missing assembly!
#endif

long pthread_cancelable_syscall(long a1, long a2, long a3, long a4, long a5, long a6, int nr)
{
	volatile uint8_t* slot = cancel_slot();

	if (slot == NULL)
		return linux_syscall(a1, a2, a3, a4, a5, a6, nr);

	if (__atomic_load_n(&slot[CANCEL_MARKED_BYTE], __ATOMIC_RELAXED) != 0)
	{
		if (dserver_rpc_pthread_testcancel() == 0)
			return -LINUX_EINTR;

		// cancellation is disabled (or the cancel was already acted on), and darlingserver
		// neither marks nor interrupts a marked thread again, so there is nothing to watch for
		return linux_syscall(a1, a2, a3, a4, a5, a6, nr);
	}

	// a signal handler may get here while the thread it interrupted is in here too
	uint8_t was_waiting = __atomic_load_n(&slot[CANCEL_WAITING_BYTE], __ATOMIC_RELAXED);

	// darlingserver sets the marked byte before it checks the waiting byte, and we set the waiting byte
	// before we check the marked byte: either we see the mark, or darlingserver sees us waiting and
	// interrupts us (or both)
	__atomic_store_n(&slot[CANCEL_WAITING_BYTE], 1, __ATOMIC_RELAXED);
	__atomic_thread_fence(__ATOMIC_SEQ_CST);

	long ret = linux_syscall_cancelable(a1, a2, a3, a4, a5, a6, nr, &slot[CANCEL_MARKED_BYTE]);

	__atomic_store_n(&slot[CANCEL_WAITING_BYTE], was_waiting, __ATOMIC_RELAXED);

	return ret;
}

uintptr_t pthread_cancelable_syscall_interrupted(uintptr_t ip)
{
	if (ip >= linux_syscall_cancelable_window[0] && ip < linux_syscall_cancelable_window[1] && cancel_marked())
		return linux_syscall_cancelable_window[2];
	return ip;
}
