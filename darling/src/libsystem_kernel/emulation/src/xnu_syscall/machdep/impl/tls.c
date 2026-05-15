#include <darling/emulation/xnu_syscall/machdep/impl/tls.h>

#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

#define ARCH_SET_GS	0x1001

#if defined(__aarch64__)
// On ARM64 Linux, there's only one user TLS register (TPIDR_EL0), already
// used by Linux pthreads. We use a compiler __thread variable to store the
// Darwin TSD base instead of a dedicated register.
static __thread void* darwin_tsd_base;

void* sys_thread_get_tsd_base(void)
{
	return darwin_tsd_base;
}
#endif

void sys_thread_set_tsd_base(void* ptr, int unk)
{
#ifdef __x86_64__
	LINUX_SYSCALL(__NR_arch_prctl, ARCH_SET_GS, ptr);
#elif defined(__i386__)
	struct user_desc
	{
		unsigned int  entry_number;
		unsigned long base_addr;
		unsigned int  limit;
		unsigned int  seg_32bit:1;
		unsigned int  contents:2;
		unsigned int  read_exec_only:1;
		unsigned int  limit_in_pages:1;
		unsigned int  seg_not_present:1;
		unsigned int  useable:1;
	};

	struct user_desc desc;
	static int entry_number = -1;

	desc.base_addr = (unsigned long) ptr;
	desc.limit = 4096;
	desc.seg_32bit = 1;
	desc.contents = 0;
	desc.read_exec_only = 0;
	desc.limit_in_pages = 1;
	desc.seg_not_present = 0;
	desc.useable = 1;
	desc.entry_number = entry_number;

	LINUX_SYSCALL(__NR_set_thread_area, &desc);

	entry_number = desc.entry_number;
	__asm__ ("movl %0, %%fs" :: "r" (desc.entry_number*8 + 3));
#elif defined(__aarch64__)
	darwin_tsd_base = ptr;
#endif
}
