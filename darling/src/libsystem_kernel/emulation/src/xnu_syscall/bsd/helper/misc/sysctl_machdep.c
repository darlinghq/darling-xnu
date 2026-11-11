#include <darling/emulation/xnu_syscall/bsd/helper/misc/sysctl_machdep.h>

#include <sys/errno.h>
#include <alloca.h>

#include <darling/emulation/common/simple.h>
#include <darling/emulation/conversion/fcntl/open.h>
#include <darling/emulation/xnu_syscall/bsd/impl/fcntl/open.h>
#include <darling/emulation/xnu_syscall/bsd/helper/misc/readline.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/close.h>
#include <darling/emulation/xnu_syscall/bsd/helper/misc/platform_identity.h>

// The configured CPU brand string (`CPUBrandString`), if there is one.
static const char* configured_brand_string(void)
{
	const dserver_platform_identity_t* identity = platform_identity();

	if (!identity || identity->cpu_brand_string[0] == '\0')
		return NULL;
	return identity->cpu_brand_string;
}

#if defined(__aarch64__) || defined(__arm64__)
// ARM64: provide stub sysctl tables (no CPUID on ARM)

extern char *strncpy(char *dest, const char *src, __SIZE_TYPE__ n);

static sysctl_handler(handle_arm_stub)
{
	return copyout_string("ARM64", (char*) old, oldlen);
}

static sysctl_handler(handle_arm_brand_string)
{
	const char* brand = configured_brand_string();

	return copyout_string(brand ? brand : "ARM64", (char*) old, oldlen);
}

enum {
	_MACHDEP_CPU = 1000,
};

enum {
	_CPU_VENDOR = 1000,
	_CPU_BRAND_STRING,
	_CPU_CORE_COUNT,
};

static sysctl_handler(handle_core_count);

const struct known_sysctl sysctls_machdep_cpu[] = {
	{ .oid = _CPU_VENDOR, .type = CTLTYPE_STRING, .exttype = "S", .name = "vendor", .handler = handle_arm_stub },
	{ .oid = _CPU_BRAND_STRING, .type = CTLTYPE_STRING, .exttype = "S", .name = "brand_string", .handler = handle_arm_brand_string },
	{ .oid = _CPU_CORE_COUNT, .type = CTLTYPE_INT, .exttype = "I", .name = "core_count", .handler = handle_core_count },
	{ .oid = -1 }
};

const struct known_sysctl sysctls_machdep[] = {
	{ .oid = _MACHDEP_CPU, .type = CTLTYPE_NODE, .exttype = "", .name = "cpu", .subctls = sysctls_machdep_cpu },
	{ .oid = -1 }
};

sysctl_handler(handle_core_count)
{
	if (!oldlen)
		return -EINVAL;
	sysctl_handle_size(sizeof(int));

	int count = 1;
	int fd = sys_open_nocancel("/proc/cpuinfo", LINUX_O_RDONLY, 0);
	if (fd < 0)
		goto out;
	count = 0;
	char line[256];
	struct simple_readline_buf rbuf;
	__simple_readline_init(&rbuf);
	while (__simple_readline(fd, &rbuf, line, sizeof(line))) {
		if (strncmp(line, "processor", 9) == 0)
			count++;
	}
	sys_close_nocancel(fd);
	if (count == 0) count = 1;
out:
	__builtin_memcpy(old, &count, sizeof(count));
	return 0;
}

#else /* x86 */

extern char *strncpy(char *dest, const char *src, __SIZE_TYPE__ n);
extern int strncmp(const char* str1, const char* str2, __SIZE_TYPE__ n);;

enum {
	_MACHDEP_CPU = 1000,
};

enum {
	_CPU_VENDOR = 1000,
	_CPU_MAX_BASIC,
	_CPU_FAMILY,
	_CPU_MODEL,
	_CPU_STEPPING,
	_CPU_BRAND_STRING,
	_CPU_FEATURES,
	_CPU_CORE_COUNT,
	_CPU_THREAD_COUNT,
	_CPU_FEATURE_BITS,
};

static sysctl_handler(handle_vendor);
static sysctl_handler(handle_max_basic);
static sysctl_handler(handle_family);
static sysctl_handler(handle_model);
static sysctl_handler(handle_stepping);
static sysctl_handler(handle_brand_string);
static sysctl_handler(handle_features);
static sysctl_handler(handle_core_count);
static sysctl_handler(handle_thread_count);
static sysctl_handler(handle_feature_bits);

const struct known_sysctl sysctls_machdep_cpu[] = {
    { .oid = _CPU_MAX_BASIC, .type = CTLTYPE_INT, .exttype = "I", .name = "max_basic", .handler = handle_max_basic },
		{ .oid = _CPU_VENDOR, .type = CTLTYPE_STRING, .exttype = "S", .name = "vendor", .handler = handle_vendor },
    { .oid = _CPU_FAMILY, .type = CTLTYPE_INT, .exttype = "I", .name = "family", .handler = handle_family },
		{ .oid = _CPU_MODEL, .type = CTLTYPE_INT, .exttype = "I", .name = "model", .handler = handle_model },
		{ .oid = _CPU_STEPPING, .type = CTLTYPE_INT, .exttype = "I", .name = "stepping", .handler = handle_stepping },
		{ .oid = _CPU_BRAND_STRING, .type = CTLTYPE_STRING, .exttype = "S", .name = "brand_string", .handler = handle_brand_string },
		{ .oid = _CPU_FEATURES, .type = CTLTYPE_STRING, .exttype = "S", .name = "features", .handler = handle_features },
		{ .oid = _CPU_CORE_COUNT, .type = CTLTYPE_INT, .exttype = "I", .name = "core_count", .handler = handle_core_count },
		{ .oid = _CPU_THREAD_COUNT, .type = CTLTYPE_INT, .exttype = "I", .name = "thread_count", .handler = handle_thread_count },
	{ .oid = _CPU_FEATURE_BITS, .type = CTLTYPE_QUAD, .exttype = "IU", .name = "feature_bits", .handler = handle_feature_bits },
	{ .oid = -1 }
};

const struct known_sysctl sysctls_machdep[] = {
	{ .oid = _MACHDEP_CPU, .type = CTLTYPE_NODE, .exttype = "", .name = "cpu", .subctls = sysctls_machdep_cpu },
	{ .oid = -1 }
};

#ifndef __cpuid
#define __cpuid(level, a, b, c, d)			\
  __asm__ ("cpuid\n\t"					\
	   : "=a" (a), "=b" (b), "=c" (c), "=d" (d)	\
	   : "0" (level))
#endif

#ifndef setup
#define setup(value)\
            unsigned int level = value; \
            unsigned int eax;           \
            unsigned int ebx;           \
            unsigned int edx;           \
            unsigned int ecx
#endif


static inline void copyout_int(int value, void* to_copy, unsigned long* to_copy_length)
{
	if (to_copy != NULL)
	{
		unsigned long to_copy_sz = sizeof(int);
		if (to_copy_length != NULL && *to_copy_length < to_copy_sz)
			to_copy_sz = *to_copy_length;
		__builtin_memcpy(to_copy, &value, to_copy_sz);
	}
	if (to_copy_length != NULL)
		*to_copy_length = sizeof(int);
}


// The CPUID leaf 1 feature flags: ECX in the upper and EDX in the lower half.
// On macOS this is a 64-bit value whose format string is "IU" (observed on
// macOS 27 under Rosetta); there is no machdep.cpu.signature there.
sysctl_handler(handle_feature_bits)
{
    if (!oldlen) return -EINVAL;
    if (_new) return -EPERM;
    sysctl_handle_size(sizeof(uint64_t));
    setup(1);
    __cpuid(level, eax, ebx, ecx, edx);
    *(uint64_t*)old = ((uint64_t)ecx << 32) | edx;
    return 0;
}

sysctl_handler(handle_vendor)
{
	unsigned int level = 0;
    unsigned int eax = 0;
	union
	{
		struct
		{
	    	unsigned int ebx;
		    unsigned int edx;
	    	unsigned int ecx;
		};
		char name[13];
	} v;

    __cpuid(level, eax, v.ebx, v.ecx, v.edx);

	v.name[12] = 0;
	return copyout_string(v.name, (char*) old, oldlen);
}

sysctl_handler(handle_max_basic)
{
	setup(0);
	__cpuid(level, eax, ebx, ecx, edx);

	copyout_int(eax, old, oldlen);

	return 0;
}

sysctl_handler(handle_family)
{
	setup(1);
	__cpuid(level, eax, ebx, ecx, edx);

	unsigned int family = (eax >> 8) & 0x0F;
	unsigned int extfamily = (eax >> 20) & 0xFF;
	if (family == 0x0F)
		family += extfamily;

	copyout_int(family, old, oldlen);

	return 0;
}

sysctl_handler(handle_model)
{
	setup(1);
	__cpuid(level, eax, ebx, ecx, edx);

	unsigned int family = (eax >> 8) & 0x0F;
	unsigned int model = (eax >> 4) & 0x0F;
	unsigned int extmodel = (eax >> 16) & 0x0F;
	if (family == 0x0F || family == 0x06)
		model += (extmodel << 4);

	copyout_int(model, old, oldlen);

	return 0;
}

sysctl_handler(handle_stepping)
{
	setup(1);
	__cpuid(level, eax, ebx, ecx, edx);

	unsigned int stepping = eax & 0x0F;

	copyout_int(stepping, old, oldlen);

	return 0;
}

sysctl_handler(handle_brand_string)
{
    const char* brand = configured_brand_string();
    if (brand != NULL)
        return copyout_string(brand, (char*) old, oldlen);

    setup(0x80000000);

    __cpuid(level,eax,ebx, ecx, edx);

    if (eax < 0x80000004) // the information is not implemented
        return 2;



    union
    {
        unsigned int brand[12];
        char name[49];
    } v;

		for (int i = 1; i < 4; i++)
		{
				__cpuid(0x80000001 + i, eax, ebx, ecx, edx);
				v.brand[0x0+(i-1)*4] = eax;
				v.brand[0x1+(i-1)*4] = ebx;
				v.brand[0x2+(i-1)*4] = ecx;
				v.brand[0x3+(i-1)*4] = edx;
		}

    v.name[48] = 0;
    return copyout_string(v.name, (char*) old, oldlen);
}

sysctl_handler(handle_features)
{
	setup(1);

	static const char features[][7] = {"FPU","VME", "DE", "PSE", "TSC", "MSR", "PAE", "MCE", "CX8", "APIC", "","SEP","MTRR","PGE",
						"MCA", "CMOV", "PAT", "PSE-36", "PSN", "CLFSH", "", "DS", "ACPI", "MMX", "FXSR", "SSE", "SSE2", "SS",
						"HTT", "TM","IA64","PBE"};

	__cpuid(1, eax, ebx, ecx, edx);

	char buf[512];
	unsigned long len = 0;

	for (int i = 0; i < 32; i++)
	{
		if (i == 10 || i == 20)
			continue;

		if (edx & (1 << i))
		{
			if (len > 0 && len + 1 < sizeof(buf))
				buf[len++] = ' ';

			unsigned long flen = __simple_strlen(features[i]);
			if (len + flen < sizeof(buf))
			{
				__builtin_memcpy(buf + len, features[i], flen);
				len += flen;
			}
		}
	}
	buf[len] = '\0';

	return copyout_string(buf, (char*)old, oldlen);
}


// Reads a per-package count ("cpu cores" or "siblings") from /proc/cpuinfo; like
// machdep.cpu.core_count and machdep.cpu.thread_count on XNU, these describe one package.
// it's easier to use /proc/cpuinfo for this.
// using cpuid directly is more difficult because AMD processors don't use the same method as Intel processors.
// TODO: i doubt these counts are ever going to change.
//       this is something that could be cached if we had some way of doing something only once in libsystem_kernel (i.e. a simple version of dispatch_once).
static uint32_t cpuinfo_package_count(const char* field)
{
	int infofd = sys_open_nocancel("/proc/cpuinfo", BSD_O_RDONLY, 0);
	struct rdline_buffer rbuf;
	const char* line;
	size_t field_length = __simple_strlen(field);

	// a reasonable default
	uint32_t count = 1;

	if (infofd < 0)
		return count;

	_readline_init(&rbuf);

	while ((line = _readline(infofd, &rbuf)) != NULL) {
		if (strncmp(line, field, field_length) != 0) {
			continue;
		}

		// great, we've got the field
		// now find the actual number
		while (*line != ':' && *line != '\0')
			++line;

		// shouldn't happen, but just in case
		if (*line == '\0')
			continue;

		++line; // skip the colon
		if (*line == '\0')
			continue;

		++line; // skip the space
		if (*line == '\0')
			continue;

		count = __simple_atoi(line, NULL);
		break;
	}

	close_internal(infofd);

	if (count == 0)
		count = 1;
	return count;
}

static void copyout_count(uint32_t count, void* old, unsigned long* oldlen)
{
	if (old != NULL && oldlen && *oldlen >= sizeof(uint32_t))
		*(uint32_t*)old = count;
	if (oldlen)
		*oldlen = sizeof(uint32_t);
}

// the number of cores in a package
sysctl_handler(handle_core_count) {
	copyout_count(cpuinfo_package_count("cpu cores"), old, oldlen);
	return 0;
}

// the number of logical CPUs (hardware threads) in a package
sysctl_handler(handle_thread_count) {
	copyout_count(cpuinfo_package_count("siblings"), old, oldlen);
	return 0;
}

#endif /* __aarch64__ || __arm64__ */
