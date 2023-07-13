#include "bsdthread_register.h"
#include "../base.h"
#include "../errno.h"
#include <sys/errno.h>
#include "../signal/sigexc.h"
#include <darlingserver/rpc.h>
#include <stdint.h>
#include <pthread/tsd_private.h>
#include <linux-syscalls/linux.h>
#include "../simple.h"

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

void pthread_entry_point_wrapper(void* self, int thread_port, void* funptr,
		void* funarg, unsigned long stack_addr, unsigned int flags)
{
	sigexc_thread_setup();

	uintptr_t pthread_handle = (unsigned long) self;
	uintptr_t dispatch_qaddr = pthread_handle + g_pth_regdata.tsd_offset + g_pth_regdata.dispatch_queue_offset;
	if (dserver_rpc_set_thread_handles(pthread_handle, dispatch_qaddr) < 0) {
		__simple_abort();
	}

#if __x86_64__
	register void*         arg1 asm("rdi") = self;
	register int           arg2 asm("esi") = thread_port;
	register void*         arg3 asm("rdx") = funptr;
	register void*         arg4 asm("rcx") = funarg;
	register unsigned long arg5 asm("r8")  = stack_addr;
	register unsigned int  arg6 asm("r9d") = flags;
#elif __i386__
	// All arguments are stored into the stack
#endif

// `libpthread/src/pthread_asm.S` does not expect `_thread_start` to
// be called the normal way (where the return address is added).
#if __x86_64__
	__asm__ __volatile__ (
		"jmpq *%[pthread_entry_point]\n"
		::
		"r"(arg1),"r"(arg2),"r"(arg3),"r"(arg4),"r"(arg5),"r"(arg6),
		[pthread_entry_point] "r"(pthread_entry_point)
	);
#elif __i386__
	__asm__ __volatile__ (
		// Make sure stack is 16 aligned
		"sub $8, %%esp\n"
		// Function arguments
		"pushl %[arg6]\n"
		"pushl %[arg5]\n"
		"pushl %[arg4]\n"
		"pushl %[arg3]\n"
		"pushl %[arg2]\n"
		"pushl %[arg1]\n"
		// Jump to _thread_start
		"jmpl *%[pthread_entry_point]\n" ::
		
		[arg1] "m"(self),
		[arg2] "m"(thread_port),
		[arg3] "m"(funptr),
		[arg4] "m"(funarg),
		[arg5] "m"(stack_addr),
		[arg6] "m"(flags),
		
		[pthread_entry_point] "r"(pthread_entry_point)
	);
#else
	#error "Missing assembly for architecture"
	// pthread_entry_point(self, thread_port, funptr, funarg, stack_addr, flags);
#endif
}

void wqueue_entry_point_wrapper(void* self, int thread_port, void* stackaddr,
		void* item, int reuse, int nevents)
{
	sigexc_thread_setup();

#if __x86_64__
	register void* arg1 asm("rdi") = self;
	register int   arg2 asm("esi") = thread_port;
	register void* arg3 asm("rdx") = stackaddr;
	register void* arg4 asm("rcx") = item;
	register int   arg5 asm("r8d") = reuse;
	register int   arg6 asm("r9d") = nevents;
#elif __i386__
	// All arguments are stored into the stack
#endif

// `libpthread/src/pthread_asm.S` does not expect `_start_wqthread` to
// be called the normal way (where the return address is added).
#if __x86_64__
	__asm__ __volatile__ (
		"jmpq *%[wqueue_entry_point]\n"
		::
		"r"(arg1),"r"(arg2),"r"(arg3),"r"(arg4),"r"(arg5),"r"(arg6),
		[wqueue_entry_point] "r"(wqueue_entry_point)
	);
#elif __i386__
	__asm__ __volatile__ (
		// Make sure stack is 16 aligned
		"sub $8, %%esp\n"
		// Function arguments
		"pushl %[arg6]\n"
		"pushl %[arg5]\n"
		"pushl %[arg4]\n"
		"pushl %[arg3]\n"
		"pushl %[arg2]\n"
		"pushl %[arg1]\n"
		// Jump to _start_wqthread
		"jmpl *%[wqueue_entry_point]\n" ::
		
		[arg1] "m"(self),
		[arg2] "m"(thread_port),
		[arg3] "m"(stackaddr),
		[arg4] "m"(item),
		[arg5] "m"(reuse),
		[arg6] "m"(nevents),
		
		[wqueue_entry_point] "r"(wqueue_entry_point)
	);
#else
	#error "Missing assembly for architecture"
	// wqueue_entry_point(self, thread_port, stackaddr, item, reuse, nevents);
#endif

	// _start_wqthread never returns
	__builtin_unreachable();
}
