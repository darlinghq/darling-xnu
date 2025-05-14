#include <darling/emulation/xnu_syscall/bsd/impl/bsdthread/bsdthread_register.h>

#include <sys/errno.h>
#include <stdint.h>
#include <pthread/tsd_private.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/signal/sigexc.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/common/simple.h>

#include <darlingserver/rpc.h>

int pthread_obj_size;
bsdthread_entry_t pthread_entry_point;
bsdwqthread_entry_t wqueue_entry_point;

#define PTHREAD_FEATURE_DISPATCHFUNC 1
#define PTHREAD_FEATURE_QOS_MAINTENANCE 0x10
#define PTHREAD_FEATURE_QOS_DEFAULT 0x40000000
#define WORKQ_FEATURE_FINEPRIO 0x02
#define WORKQ_FEATURE_KEVENT 0x40

struct _pthread_registration_data {
	uint64_t version;

	uint64_t dispatch_queue_offset;
	uint64_t main_qos;
	uint32_t tsd_offset;
	uint32_t return_to_kernel_offset;
	uint32_t mach_thread_self_offset;
} __attribute__ ((packed));

static struct _pthread_registration_data g_pth_regdata;

long sys_bsdthread_register(void* thread_start, void* wqthread, int pthsize,
		const struct _pthread_registration_data* pth_regdata,
		unsigned long pth_regdata_len, unsigned long long dpq_offset)
{
	pthread_obj_size = pthsize;
	pthread_entry_point = (bsdthread_entry_t) thread_start;
	wqueue_entry_point = (bsdwqthread_entry_t) wqthread;
	g_pth_regdata = *pth_regdata;

	uintptr_t pthread_handle = _pthread_getspecific_direct(_PTHREAD_TSD_SLOT_PTHREAD_SELF);
	uintptr_t dispatch_qaddr = pthread_handle + g_pth_regdata.tsd_offset + g_pth_regdata.dispatch_queue_offset;
	if (dserver_rpc_set_thread_handles(pthread_handle, dispatch_qaddr) < 0) {
		__simple_abort();
	}

	return /* WORKQ_FEATURE_KEVENT | WORKQ_FEATURE_FINEPRIO | PTHREAD_FEATURE_QOS_MAINTENANCE 
		| PTHREAD_FEATURE_DISPATCHFUNC | PTHREAD_FEATURE_QOS_DEFAULT */ 0;
}

// Before you make any changes to the assembly code, I strongly recommend looking at the `_thread_start`
// code. `_thread_start` does not follow the usual calling conventions you would expect. You can find the
// source in `libpthread/src/pthread_asm.S`.
void pthread_entry_point_wrapper(void* self, int thread_port, void* funptr,
		void* funarg, unsigned long stack_addr, unsigned int flags)
{
	sigexc_thread_setup();

	uintptr_t pthread_handle = (unsigned long) self;
	uintptr_t dispatch_qaddr = pthread_handle + g_pth_regdata.tsd_offset + g_pth_regdata.dispatch_queue_offset;
	if (dserver_rpc_set_thread_handles(pthread_handle, dispatch_qaddr) < 0) {
		__simple_abort();
	}

	// No additional function calls should occur beyond this point. Otherwise, we will risk our
	// registers being call-clobbered. I recommend reading the following doc for more details:
	// https://gcc.gnu.org/onlinedocs/gcc/Local-Register-Variables.html
#if __x86_64__
	register void*         arg1 asm("rdi") = self;
	register int           arg2 asm("esi") = thread_port;
	register void*         arg3 asm("rdx") = funptr;
	register void*         arg4 asm("rcx") = funarg;
	register unsigned long arg5 asm("r8")  = stack_addr;
	register unsigned int  arg6 asm("r9d") = flags;
#elif __i386__
	register void*         arg1 asm("eax") = self;
	register int           arg2 asm("ebx") = thread_port;
	register void*         arg3 asm("ecx") = funptr;
	register void*         arg4 asm("edx") = funarg;
	register unsigned long arg5 asm("edi") = stack_addr;
	register unsigned int  arg6 asm("esi") = flags;
#endif

#if __x86_64__
	__asm__ __volatile__ (
		// `_thread_start` does not expect the stack to be aligned (using the 
		// call instruction will add the return address to the stack, causing 
		// it to be aligned).
		"jmpq *%[pthread_entry_point]\n"
		::
		// Arguments follow the usual x86_64 calling conventions.
		"r"(arg1),"r"(arg2),"r"(arg3),"r"(arg4),"r"(arg5),"r"(arg6),
		[pthread_entry_point] "r"(pthread_entry_point)
	);
#elif __i386__
	__asm__ __volatile__ (
		// `_thread_start` does not expect the stack to be aligned.
		"jmpl *%[pthread_entry_point]\n"
		::
		// `_thread_start` does not follow the i386 calling conventions
		// for arguments. Instead of storing the arguments into the stack, 
		// the arguments are stored in the registers.
		"r"(arg1),"r"(arg2),"r"(arg3),"r"(arg4),"r"(arg5),"r"(arg6),
		[pthread_entry_point] "rmi"(pthread_entry_point)
	);
#else
	#error "Missing assembly for architecture"
	// pthread_entry_point(self, thread_port, funptr, funarg, stack_addr, flags);
#endif

	// `_thread_start` actually does actually return, but since we are jumping to 
	// (instead of calling) `_thread_start`, `_thread_start` will return to the 
	// method that called this method.
	__builtin_unreachable();
}

void wqueue_entry_point_wrapper(void* self, int thread_port, void* stackaddr,
		void* item, int reuse, int nevents)
{
	sigexc_thread_setup();
	wqueue_entry_point_asm_jump(self, thread_port, stackaddr, item, reuse, nevents);
}

// Before you make any changes to the assembly code, I strongly recommend looking at the `_start_wqthread`
// code. `_start_wqthread` does not follow the usual calling conventions you would expect. You can find the
// source in `libpthread/src/pthread_asm.S`.
void wqueue_entry_point_asm_jump(void* self, int thread_port, void* stackaddr,
		void* item, int reuse, int nevents) {
	
	// No additional function calls should occur beyond this point. Otherwise, we will risk our
	// registers being call-clobbered. I recommend reading the following doc for more details:
	// https://gcc.gnu.org/onlinedocs/gcc/Local-Register-Variables.html
#if __x86_64__
	register void* arg1 asm("rdi") = self;
	register int   arg2 asm("esi") = thread_port;
	register void* arg3 asm("rdx") = stackaddr;
	register void* arg4 asm("rcx") = item;
	register int   arg5 asm("r8d") = reuse;
	register int   arg6 asm("r9d") = nevents;
#elif __i386__
	register void* arg1 asm("eax") = self;
	register int   arg2 asm("ebx") = thread_port;
	register void* arg3 asm("ecx") = stackaddr;
	register void* arg4 asm("edx") = item;
	register int   arg5 asm("edi") = reuse;
	register int   arg6 asm("esi") = nevents;
#endif

#if __x86_64__
	__asm__ __volatile__ (
		// `_start_wqthread` does not expect the stack to be aligned (using the 
		// call instruction will add the return address to the stack, causing 
		// it to be aligned).
		"jmpq *%[wqueue_entry_point]\n"
		::
		// Arguments follow the usual x86_64 calling conventions.
		"r"(arg1),"r"(arg2),"r"(arg3),"r"(arg4),"r"(arg5),"r"(arg6),
		[wqueue_entry_point] "r"(wqueue_entry_point)
	);
#elif __i386__
	__asm__ __volatile__ (
		// `_start_wqthread` does not expect the stack to be aligned.
		"jmpl *%[wqueue_entry_point]\n"
		::
		// `_start_wqthread` does not follow the i386 calling conventions
		// for arguments. Instead of storing the arguments into the stack, 
		// the arguments are stored in the registers.
		"r"(arg1),"r"(arg2),"r"(arg3),"r"(arg4),"r"(arg5),"r"(arg6),
		[wqueue_entry_point] "rmi"(wqueue_entry_point)
	);
#else
	#error "Missing assembly for architecture"
	// wqueue_entry_point(self, thread_port, stackaddr, item, reuse, nevents);
#endif

	// _start_wqthread never returns
	__builtin_unreachable();
}
