// Copyright (C) 2026 Lubos Dolezel
// kern.hv_vmm_present: read-only int, 0 or 1. On x86 it must agree with the
// CPUID hypervisor bit (leaf 1, ECX bit 31). macOS 26.7.3 x86_64 reports an int
// of size 4 (value 1 inside a VM).
#include <stdio.h>
#include <errno.h>
#include <sys/sysctl.h>

static int failures;
#define CHECK(c) do { if (!(c)) { printf("FAIL: %s (line %d, errno %d)\n", #c, __LINE__, errno); failures++; } } while (0)

int main(void)
{
	int v = -1;
	size_t len = sizeof v;
	CHECK(sysctlbyname("kern.hv_vmm_present", &v, &len, NULL, 0) == 0);
	CHECK(len == sizeof(int));
	CHECK(v == 0 || v == 1);

	size_t want = 0;
	CHECK(sysctlbyname("kern.hv_vmm_present", NULL, &want, NULL, 0) == 0);
	CHECK(want == sizeof(int));

	// read-only
	int one = 1;
	errno = 0;
	CHECK(sysctlbyname("kern.hv_vmm_present", NULL, NULL, &one, sizeof one) == -1);

#if defined(__i386__) || defined(__x86_64__)
	unsigned int eax = 1, ebx, ecx = 0, edx;
	__asm__ volatile ("cpuid" : "+a" (eax), "=b" (ebx), "+c" (ecx), "=d" (edx));
	CHECK(v == (int) ((ecx >> 31) & 1));
#endif

	if (failures) return 1;
	printf("PASS\n");
	return 0;
}
