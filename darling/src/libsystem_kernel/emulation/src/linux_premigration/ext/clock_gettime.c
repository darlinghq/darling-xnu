#include <darling/emulation/linux_premigration/ext/sys/linux_time.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/common/base.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

#include <stdbool.h>
#include <stddef.h>
#include <elfcalls.h>

extern long cerror(int __err);

extern struct elf_calls* _elfcalls;

#if defined(__x86_64__)
// Every Darling process is a Linux process, so the host kernel's vDSO is mapped
// into it. Its clock_gettime reads the same clocksource as the system call (and
// itself falls back to the system call when the clocksource cannot be read from
// user space), so it returns the same values without entering the kernel.
//
// The entry point is looked up through the host's dynamic loader. That happens
// in mach_driver_init(), before the first clock read could come from a signal
// handler; afterwards a read is an atomic load and an indirect call.

// NULL: not looked up yet; VDSO_ABSENT: not available; otherwise the entry point
static void* vdso_clock_gettime_entry;
#define VDSO_ABSENT ((void*) -1)

static void* vdso_lookup(struct elf_calls* calls, void* vdso, const char* name)
{
	void* entry = calls->dlsym(vdso, name);

	// some glibc versions only find the vDSO's symbols by their version (LINUX_2.6)
	if (entry == NULL)
	{
		void* libc = calls->dlopen("libc.so.6");
		void* (*dlvsym)(void*, const char*, const char*) = libc ? calls->dlsym(libc, "dlvsym") : NULL;
		if (dlvsym != NULL)
			entry = dlvsym(vdso, name, "LINUX_2.6");
	}

	return entry;
}

static void* vdso_clock_gettime_resolve(void)
{
	void* entry = __atomic_load_n(&vdso_clock_gettime_entry, __ATOMIC_ACQUIRE);
	struct elf_calls* calls = _elfcalls;

	if (entry != NULL)
		return entry == VDSO_ABSENT ? NULL : entry;

	// without the loader interface (too early in process startup), try again next time
	if (calls == NULL || calls->dlopen == NULL || calls->dlsym == NULL)
		return NULL;

	// only one thread looks it up; the others use the system call meanwhile
	if (!__atomic_compare_exchange_n(&vdso_clock_gettime_entry, &entry, VDSO_ABSENT, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE))
		return entry == VDSO_ABSENT ? NULL : entry;

	void* vdso = calls->dlopen("linux-vdso.so.1");
	entry = vdso ? vdso_lookup(calls, vdso, "__vdso_clock_gettime") : NULL;
	__atomic_store_n(&vdso_clock_gettime_entry, entry ? entry : VDSO_ABSENT, __ATOMIC_RELEASE);
	return entry;
}

// The vDSO is System V ABI code; realign the stack in case a caller did not keep
// the alignment the ABI requires (noinline keeps the realignment if this is inlined).
__attribute__((noinline))
__attribute__((force_align_arg_pointer))
static int vdso_clock_gettime_call(void* entry, int clockid, struct timespec* res)
{
	return ((int (*)(int, struct timespec*)) entry)(clockid, res);
}

int __darling_vdso_clock_gettime(int clockid, struct timespec* res)
{
	void* entry = vdso_clock_gettime_resolve();

	if (entry == NULL)
		return -1;

	return vdso_clock_gettime_call(entry, clockid, res) == 0 ? 0 : -1;
}

void __darling_vdso_clock_init(void)
{
	vdso_clock_gettime_resolve();
}
#else
int __darling_vdso_clock_gettime(int clockid, struct timespec* res)
{
	return -1;
}

void __darling_vdso_clock_init(void)
{
}
#endif

VISIBLE
int clock_gettime (int __clockid, struct timespec* __res)
{
	int rv;

	if (__darling_vdso_clock_gettime(__clockid, __res) == 0)
		return 0;

	// the system call also reports the errors (e.g. an unknown clock)
	rv = LINUX_SYSCALL(__NR_clock_gettime, __clockid, __res);
	if (rv < 0)
	{
		cerror(errno_linux_to_bsd(-rv));
		return -1;
	}

	return rv;
}
