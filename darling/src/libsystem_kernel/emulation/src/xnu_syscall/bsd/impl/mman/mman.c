#include <darling/emulation/xnu_syscall/bsd/impl/mman/mman.h>
#include <darling/emulation/common/dc_mseal.h>

#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <mach/vm_page_size.h>
#include <darling/emulation/conversion/mman/duct_mman.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/conversion/duct_errno.h>
#include <darling/emulation/common/base.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

static int prot_bsd_to_linux(int prot)
{
	int linux_prot = 0;

	if (prot & PROT_READ)
		linux_prot |= LINUX_PROT_READ;
	if (prot & PROT_WRITE)
		linux_prot |= LINUX_PROT_WRITE;
	if (prot & PROT_EXEC)
		linux_prot |= LINUX_PROT_EXEC;

	return linux_prot;
}

#if defined(__aarch64__) || defined(__arm64__)
/* XNU osfmk/mach/arm/vm_param.h: macOS MACH_VM_MAX_ADDRESS_RAW. */
#define DARWIN_ARM64_MMAP_LIMIT 0x00007ffffe000000ULL
#define LOW_VA_GRANULE 0x1000000ULL

static void* mmap_low(unsigned long len, int prot, int flags, int fd, long pos)
{
	/* Share the old Mach ANYWHERE fallback policy with direct BSD mappings. */
	static uintptr_t next_low_vm_addr = 0x500000000ULL;
	uintptr_t span;
	if (len > DARWIN_ARM64_MMAP_LIMIT - (LOW_VA_GRANULE - 1))
		return (void*)-LINUX_ENOMEM;
	span = (len + LOW_VA_GRANULE - 1) & ~(LOW_VA_GRANULE - 1);
	for (;;) {
		uintptr_t candidate = __atomic_load_n(&next_low_vm_addr, __ATOMIC_RELAXED);
		if (candidate >= DARWIN_ARM64_MMAP_LIMIT || span > DARWIN_ARM64_MMAP_LIMIT - candidate)
			return (void*)-LINUX_ENOMEM;
		if (!__atomic_compare_exchange_n(&next_low_vm_addr, &candidate, candidate + span,
				0, __ATOMIC_RELAXED, __ATOMIC_RELAXED))
			continue;
		void* result = (void*)LINUX_SYSCALL6(__NR_mmap, (void*)candidate, len, prot,
				flags | LINUX_MAP_FIXED_NOREPLACE, fd, pos);
		if (result == (void*)candidate)
			return result;
		if (result == (void*)-LINUX_EEXIST)
			continue;
		if ((uintptr_t)result < (uintptr_t)-4096) {
			/* Older kernels can ignore NOREPLACE and treat candidate as a hint. */
			LINUX_SYSCALL2(__NR_munmap, result, len);
			return (void*)-LINUX_EINVAL;
		}
		return result;
	}
}
#endif

void* sys_mmap(void* start, unsigned long len, int prot, int flags, int fd, long pos)
{
	int linux_prot;
	int linux_flags = 0;
	void* ret;

	linux_prot = prot_bsd_to_linux(prot);

	// if (flags & MAP_32BIT)
	// 	linux_flags |= LINUX_MAP_32BIT;
	if (flags & MAP_ANON)
		linux_flags |= LINUX_MAP_ANONYMOUS;
	if (flags & MAP_FIXED)
		linux_flags |= LINUX_MAP_FIXED;
	if (flags & MAP_FIXED_NOREPLACE)
		linux_flags |= LINUX_MAP_FIXED_NOREPLACE;
	if (flags & MAP_SHARED)
		linux_flags |= LINUX_MAP_SHARED;
	if (flags & MAP_PRIVATE)
		linux_flags |= LINUX_MAP_PRIVATE;
	// if (flags & MAP_STACK)
	// 	linux_flags |= LINUX_MAP_STACK;

#if defined(__aarch64__) || defined(__arm64__)
	if ((linux_flags & (LINUX_MAP_FIXED | LINUX_MAP_FIXED_NOREPLACE)) &&
			((uintptr_t)start >= DARWIN_ARM64_MMAP_LIMIT || len > DARWIN_ARM64_MMAP_LIMIT - (uintptr_t)start))
		return (void*)(long)-EINVAL;
#endif

#ifdef __NR_mmap2
#ifdef __i386__
	linux_flags |= LINUX_MAP_32BIT;
#endif
	ret = (void*) LINUX_SYSCALL6(__NR_mmap2, start, len, linux_prot, linux_flags, fd, pos / 4096);
#else
	ret = (void*) LINUX_SYSCALL6(__NR_mmap, start, len, linux_prot, linux_flags, fd, pos);
#endif

#if defined(__aarch64__) || defined(__arm64__)
	if (!(linux_flags & (LINUX_MAP_FIXED | LINUX_MAP_FIXED_NOREPLACE)) &&
			(uintptr_t)ret < (uintptr_t)-4096 &&
			((uintptr_t)ret >= DARWIN_ARM64_MMAP_LIMIT || len > DARWIN_ARM64_MMAP_LIMIT - (uintptr_t)ret)) {
		LINUX_SYSCALL2(__NR_munmap, ret, len);
		ret = mmap_low(len, linux_prot, linux_flags, fd, pos);
	}
#endif

	if ((unsigned long)ret > (unsigned long) -4096)
		ret = (void*)(long) errno_linux_to_bsd((int) ret);

	return ret;
}

long sys_munmap(void* addr, unsigned long len)
{
	int ret;

	ret = LINUX_SYSCALL2(__NR_munmap, addr, len);
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}

long sys_mprotect(void* addr, unsigned long len, int prot)
{
	int linux_prot, ret;

	linux_prot = prot_bsd_to_linux(prot);

	ret = LINUX_SYSCALL3(__NR_mprotect, addr, len, linux_prot);
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}

long sys_mincore(void* addr, unsigned long len, unsigned char* vec)
{
	int ret;

	ret = LINUX_SYSCALL3(__NR_mincore, addr, len, vec);
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}

long sys_mlock(void* addr, unsigned long len)
{
	int ret;

	ret = LINUX_SYSCALL2(__NR_mlock, addr, len);
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}

long sys_munlock(void* addr, unsigned long len)
{
	int ret;

	ret = LINUX_SYSCALL2(__NR_munlock, addr, len);
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}

int __darling_mseal_range(void* addr, unsigned long len, int prot)
{
	unsigned long page_mask = vm_page_size - 1;
	void* copy;
	long ret;

	if (vm_page_size == 0 || len == 0 || ((uintptr_t)addr & page_mask) != 0 ||
			len > ~0ul - page_mask || !(prot & PROT_READ) || (prot & PROT_WRITE))
		return -EINVAL;
	len = (len + page_mask) & ~page_mask;

	copy = sys_mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
	if ((uintptr_t)copy >= (uintptr_t)-4095)
		return (int)(intptr_t)copy;

	memcpy(copy, addr, len);

	ret = sys_mprotect(copy, len, prot);
	if (ret < 0)
		goto fail;

	// move the copy over the original mapping
	ret = LINUX_SYSCALL(__NR_mremap, copy, len, len, LINUX_MREMAP_MAYMOVE | LINUX_MREMAP_FIXED, addr);
	if (ret < 0 && ret >= -4095) {
		ret = errno_linux_to_bsd(ret);
		goto fail;
	}

	ret = LINUX_SYSCALL(__NR_mseal, addr, len, 0);
	if (ret < 0)
		return errno_linux_to_bsd(ret);
	return 0;

fail:
	sys_munmap(copy, len);
	return (int)ret;
}
