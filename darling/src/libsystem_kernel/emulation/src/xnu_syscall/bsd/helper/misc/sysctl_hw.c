#include <darling/emulation/xnu_syscall/bsd/helper/misc/sysctl_hw.h>

#include <mach/host_info.h>
#include <mach/machine.h>
#include <mach/mach_init.h>
#include <sys/errno.h>
#include <stdint.h>

#include <darling/emulation/xnu_syscall/bsd/helper/misc/sysctl_kern.h>
#include <darling/emulation/xnu_syscall/bsd/helper/misc/sysctl_proc.h>
#include <darling/emulation/xnu_syscall/bsd/helper/misc/platform_identity.h>
#include <darling/emulation/linux_premigration/ext/sys/utsname.h>
#include <darling/emulation/common/simple.h>
#include <darling/emulation/common/string.h>
#include <darling/emulation/common/base.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/xnu_syscall/bsd/impl/fcntl/open.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/close.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/read.h>
#include <darling/emulation/conversion/fcntl/open.h>

extern kern_return_t mach_port_deallocate(ipc_space_t task, mach_port_name_t name);
extern kern_return_t host_info(mach_port_name_t host, int itype, void* hinfo, mach_msg_type_number_t* count);

/* Darling specific */
enum {
	_HW_PHYSICAL_CPU = 1000,
	_HW_PHYSICAL_CPU_MAX,
	_HW_LOGICAL_CPU,
	_HW_LOGICAL_CPU_MAX,
	_HW_CPUTYPE,
	_HW_CPUSUBTYPE,
	_HW_CPUTHREADTYPE,
	_HW_64BITCAPABLE,
	_HW_CPUFREQUENCY,
	_HW_OPTIONAL,
	_HW_CACHECONFIG,
	_HW_CACHESIZE,
	_HW_AVAILCPU_COMPAT,
};

static sysctl_handler(handle_ncpu);
static sysctl_handler(handle_activecpu);
static sysctl_handler(handle_physicalcpu);
static sysctl_handler(handle_physicalcpu_max);
static sysctl_handler(handle_logicalcpu);
static sysctl_handler(handle_logicalcpu_max);
static sysctl_handler(handle_memsize);
static sysctl_handler(handle_pagesize);
static sysctl_handler(handle_cputype);
static sysctl_handler(handle_cpusubtype);
static sysctl_handler(handle_cputhreadtype);
static sysctl_handler(handle_cpu64bitcapable);
static sysctl_handler(handle_machine);
static sysctl_handler(handle_model);
static sysctl_handler(handle_cpufrequency);
static sysctl_handler(handle_cpufrequency_compat);
static // The Mac model identifier, the same as IOPlatformExpertDevice's "model".
sysctl_handler(handle_model)
{
	const dserver_platform_identity_t* identity = platform_identity();

	if (!identity || identity->model[0] == '\0')
		return -ENOENT;
	return copyout_string(identity->model, (char*) old, oldlen);
}

sysctl_handler(handle_cachelinesize);
static sysctl_handler(handle_cacheconfig);
static sysctl_handler(handle_cachesize);
static sysctl_handler(handle_l1icachesize);
static sysctl_handler(handle_l1dcachesize);
static sysctl_handler(handle_l2cachesize);
static sysctl_handler(handle_l3cachesize);

#if defined(__i386__) || defined(__x86_64__)
static void cpuid(unsigned int leaf, unsigned int regs[4])
{
	__asm__ ("cpuid" : "=a" (regs[0]), "=b" (regs[1]), "=c" (regs[2]), "=d" (regs[3]) : "0" (leaf), "2" (0));
}

enum { REG_EAX, REG_EBX, REG_ECX, REG_EDX };

// XCR0 state components a feature needs the OS to have enabled.
#define XSTATE_AVX 0x06 // SSE and AVX (YMM) state
#define XSTATE_MPX 0x18 // bound registers and bound config
#define XSTATE_AVX512 0xe6 // AVX state plus opmask, ZMM_Hi256 and Hi16_ZMM

// Whether the CPU has a feature, and the host kernel lets programs use it.
static int cpu_feature(unsigned int leaf, int reg, int bit, unsigned int xstate)
{
	unsigned int regs[4];

	cpuid(0, regs);
	if (regs[REG_EAX] < leaf)
		return 0;

	if (xstate != 0)
	{
		unsigned int xcr0_low, xcr0_high;

		// OSXSAVE: the kernel enabled XSAVE, so XGETBV is available.
		cpuid(1, regs);
		if (!(regs[REG_ECX] & (1u << 27)))
			return 0;
		__asm__ ("xgetbv" : "=a" (xcr0_low), "=d" (xcr0_high) : "c" (0));
		if ((xcr0_low & xstate) != xstate)
			return 0;
	}

	cpuid(leaf, regs);
	return (regs[reg] >> bit) & 1;
}

#define optional_handler(_name, _leaf, _reg, _bit, _xstate) \
	static sysctl_handler(handle_optional_##_name) \
	{ \
		sysctl_handle_size(sizeof(int)); \
		*((int*) old) = cpu_feature(_leaf, _reg, _bit, _xstate); \
		return 0; \
	}

// The hw.optional names XNU reports on Intel Macs, with their CPUID bits.
optional_handler(floatingpoint, 1, REG_EDX, 0, 0)
optional_handler(mmx, 1, REG_EDX, 23, 0)
optional_handler(sse, 1, REG_EDX, 25, 0)
optional_handler(sse2, 1, REG_EDX, 26, 0)
optional_handler(sse3, 1, REG_ECX, 0, 0)
optional_handler(supplementalsse3, 1, REG_ECX, 9, 0)
optional_handler(sse4_1, 1, REG_ECX, 19, 0)
optional_handler(sse4_2, 1, REG_ECX, 20, 0)
optional_handler(aes, 1, REG_ECX, 25, 0)
optional_handler(avx1_0, 1, REG_ECX, 28, XSTATE_AVX)
optional_handler(rdrand, 1, REG_ECX, 30, 0)
optional_handler(f16c, 1, REG_ECX, 29, XSTATE_AVX)
optional_handler(fma, 1, REG_ECX, 12, XSTATE_AVX)
optional_handler(enfstrg, 7, REG_EBX, 9, 0)
optional_handler(avx2_0, 7, REG_EBX, 5, XSTATE_AVX)
optional_handler(bmi1, 7, REG_EBX, 3, 0)
optional_handler(bmi2, 7, REG_EBX, 8, 0)
optional_handler(rtm, 7, REG_EBX, 11, 0)
optional_handler(hle, 7, REG_EBX, 4, 0)
optional_handler(adx, 7, REG_EBX, 19, 0)
optional_handler(mpx, 7, REG_EBX, 14, XSTATE_MPX)
optional_handler(sgx, 7, REG_EBX, 2, 0)
optional_handler(avx512f, 7, REG_EBX, 16, XSTATE_AVX512)
optional_handler(avx512cd, 7, REG_EBX, 28, XSTATE_AVX512)
optional_handler(avx512dq, 7, REG_EBX, 17, XSTATE_AVX512)
optional_handler(avx512bw, 7, REG_EBX, 30, XSTATE_AVX512)
optional_handler(avx512vl, 7, REG_EBX, 31, XSTATE_AVX512)
optional_handler(avx512ifma, 7, REG_EBX, 21, XSTATE_AVX512)
optional_handler(avx512vbmi, 7, REG_ECX, 1, XSTATE_AVX512)

// XNU registers these with OID_AUTO, so the numbers here are arbitrary.
static const struct known_sysctl sysctls_hw_optional[] = {
	{ .oid = 1, .type = CTLTYPE_INT, .exttype = "I", .name = "floatingpoint", .handler = handle_optional_floatingpoint },
	{ .oid = 2, .type = CTLTYPE_INT, .exttype = "I", .name = "mmx", .handler = handle_optional_mmx },
	{ .oid = 3, .type = CTLTYPE_INT, .exttype = "I", .name = "sse", .handler = handle_optional_sse },
	{ .oid = 4, .type = CTLTYPE_INT, .exttype = "I", .name = "sse2", .handler = handle_optional_sse2 },
	{ .oid = 5, .type = CTLTYPE_INT, .exttype = "I", .name = "sse3", .handler = handle_optional_sse3 },
	{ .oid = 6, .type = CTLTYPE_INT, .exttype = "I", .name = "supplementalsse3", .handler = handle_optional_supplementalsse3 },
	{ .oid = 7, .type = CTLTYPE_INT, .exttype = "I", .name = "sse4_1", .handler = handle_optional_sse4_1 },
	{ .oid = 8, .type = CTLTYPE_INT, .exttype = "I", .name = "sse4_2", .handler = handle_optional_sse4_2 },
	{ .oid = 9, .type = CTLTYPE_INT, .exttype = "I", .name = "x86_64", .handler = handle_cpu64bitcapable },
	{ .oid = 10, .type = CTLTYPE_INT, .exttype = "I", .name = "aes", .handler = handle_optional_aes },
	{ .oid = 11, .type = CTLTYPE_INT, .exttype = "I", .name = "avx1_0", .handler = handle_optional_avx1_0 },
	{ .oid = 12, .type = CTLTYPE_INT, .exttype = "I", .name = "rdrand", .handler = handle_optional_rdrand },
	{ .oid = 13, .type = CTLTYPE_INT, .exttype = "I", .name = "f16c", .handler = handle_optional_f16c },
	{ .oid = 14, .type = CTLTYPE_INT, .exttype = "I", .name = "enfstrg", .handler = handle_optional_enfstrg },
	{ .oid = 15, .type = CTLTYPE_INT, .exttype = "I", .name = "fma", .handler = handle_optional_fma },
	{ .oid = 16, .type = CTLTYPE_INT, .exttype = "I", .name = "avx2_0", .handler = handle_optional_avx2_0 },
	{ .oid = 17, .type = CTLTYPE_INT, .exttype = "I", .name = "bmi1", .handler = handle_optional_bmi1 },
	{ .oid = 18, .type = CTLTYPE_INT, .exttype = "I", .name = "bmi2", .handler = handle_optional_bmi2 },
	{ .oid = 19, .type = CTLTYPE_INT, .exttype = "I", .name = "rtm", .handler = handle_optional_rtm },
	{ .oid = 20, .type = CTLTYPE_INT, .exttype = "I", .name = "hle", .handler = handle_optional_hle },
	{ .oid = 21, .type = CTLTYPE_INT, .exttype = "I", .name = "adx", .handler = handle_optional_adx },
	{ .oid = 22, .type = CTLTYPE_INT, .exttype = "I", .name = "mpx", .handler = handle_optional_mpx },
	{ .oid = 23, .type = CTLTYPE_INT, .exttype = "I", .name = "sgx", .handler = handle_optional_sgx },
	{ .oid = 24, .type = CTLTYPE_INT, .exttype = "I", .name = "avx512f", .handler = handle_optional_avx512f },
	{ .oid = 25, .type = CTLTYPE_INT, .exttype = "I", .name = "avx512cd", .handler = handle_optional_avx512cd },
	{ .oid = 26, .type = CTLTYPE_INT, .exttype = "I", .name = "avx512dq", .handler = handle_optional_avx512dq },
	{ .oid = 27, .type = CTLTYPE_INT, .exttype = "I", .name = "avx512bw", .handler = handle_optional_avx512bw },
	{ .oid = 28, .type = CTLTYPE_INT, .exttype = "I", .name = "avx512vl", .handler = handle_optional_avx512vl },
	{ .oid = 29, .type = CTLTYPE_INT, .exttype = "I", .name = "avx512ifma", .handler = handle_optional_avx512ifma },
	{ .oid = 30, .type = CTLTYPE_INT, .exttype = "I", .name = "avx512vbmi", .handler = handle_optional_avx512vbmi },
	{ .oid = -1 }
};
#elif defined(__aarch64__)
/* AT_HWCAP and AT_HWCAP2 from /proc/self/auxv, read once. */
static int read_hwcaps(unsigned long long caps[2])
{
	/* 0: not read yet, 1: read, -1: unavailable. Threads that find it not
	 * read yet each read the file; they all get the same values, and the
	 * release store publishes the values together with the state. */
	static int state;
	static unsigned long long cached[2];
	int current = __atomic_load_n(&state, __ATOMIC_ACQUIRE);

	if (current == 0) {
		unsigned long long found[2] = { 0, 0 };
		int fd = sys_open_nocancel("/proc/self/auxv", BSD_O_RDONLY, 0);

		current = -1;
		if (fd >= 0) {
			unsigned long long pair[2];
			while (sys_read_nocancel(fd, pair, sizeof(pair)) == sizeof(pair) && pair[0] != 0 /* AT_NULL */) {
				if (pair[0] == 16 /* AT_HWCAP */) {
					found[0] = pair[1];
					current = 1;
				} else if (pair[0] == 26 /* AT_HWCAP2 */) {
					found[1] = pair[1];
				}
			}
			close_internal(fd);
		}
		if (current == 1) {
			caps[0] = found[0];
			caps[1] = found[1];
			__atomic_store_n(&cached[0], found[0], __ATOMIC_RELAXED);
			__atomic_store_n(&cached[1], found[1], __ATOMIC_RELAXED);
		}
		__atomic_store_n(&state, current, __ATOMIC_RELEASE);
		return current == 1;
	}

	if (current != 1)
		return 0;
	caps[0] = __atomic_load_n(&cached[0], __ATOMIC_RELAXED);
	caps[1] = __atomic_load_n(&cached[1], __ATOMIC_RELAXED);
	return 1;
}

/*
 * CPU features as macOS reports them under hw.optional, answered from the
 * Linux host's AT_HWCAP/AT_HWCAP2 (arm64 bits; see the kernel's
 * arch/arm64/include/uapi/asm/hwcap.h). compiler-rt's outline atomics read
 * hw.optional.armv8_1_atomics to decide whether to use LSE instructions.
 * Absent features read as 0, as on macOS.
 */
#define HWCAP1(bit) (bit)
#define HWCAP2(bit) (64 + (bit))
#define HWCAP_ALWAYS (-1)

static int hwcap_feature(int bit)
{
	unsigned long long caps[2];

	if (bit == HWCAP_ALWAYS)
		return 1;
	if (!read_hwcaps(caps))
		return 0;
	return (caps[bit / 64] >> (bit % 64)) & 1;
}

#define optional_handler(_name, _bit) \
	static sysctl_handler(handle_optional_##_name) \
	{ \
		sysctl_handle_size(sizeof(int)); \
		*((int*) old) = hwcap_feature(_bit); \
		return 0; \
	}

optional_handler(floatingpoint, HWCAP1(0))
optional_handler(neon, HWCAP1(1))
optional_handler(neon_hpfp, HWCAP1(10))
optional_handler(neon_fp16, HWCAP1(10))
optional_handler(armv8_1_atomics, HWCAP1(8))
optional_handler(armv8_crc32, HWCAP1(7))
optional_handler(armv8_2_fhm, HWCAP1(23))
optional_handler(armv8_2_sha512, HWCAP1(21))
optional_handler(armv8_2_sha3, HWCAP1(17))
optional_handler(armv8_3_compnum, HWCAP1(14))
optional_handler(arm64, HWCAP_ALWAYS)
optional_handler(FEAT_LSE, HWCAP1(8))
optional_handler(FEAT_LSE2, HWCAP1(25))
optional_handler(FEAT_CRC32, HWCAP1(7))
optional_handler(FEAT_AES, HWCAP1(3))
optional_handler(FEAT_PMULL, HWCAP1(4))
optional_handler(FEAT_SHA1, HWCAP1(5))
optional_handler(FEAT_SHA256, HWCAP1(6))
optional_handler(FEAT_SHA512, HWCAP1(21))
optional_handler(FEAT_SHA3, HWCAP1(17))
optional_handler(FEAT_FP16, HWCAP1(9))
optional_handler(FEAT_RDM, HWCAP1(12))
optional_handler(FEAT_JSCVT, HWCAP1(13))
optional_handler(FEAT_FCMA, HWCAP1(14))
optional_handler(FEAT_LRCPC, HWCAP1(15))
optional_handler(FEAT_LRCPC2, HWCAP1(26))
optional_handler(FEAT_DPB, HWCAP1(16))
optional_handler(FEAT_DotProd, HWCAP1(20))
optional_handler(FEAT_FHM, HWCAP1(23))
optional_handler(FEAT_DIT, HWCAP1(24))
optional_handler(FEAT_FlagM, HWCAP1(27))
optional_handler(FEAT_SSBS, HWCAP1(28))
optional_handler(FEAT_SB, HWCAP1(29))
optional_handler(FEAT_DPB2, HWCAP2(0))
optional_handler(FEAT_FlagM2, HWCAP2(7))
optional_handler(FEAT_FRINTTS, HWCAP2(8))
optional_handler(FEAT_I8MM, HWCAP2(13))
optional_handler(FEAT_BF16, HWCAP2(14))
optional_handler(FEAT_BTI, HWCAP2(17))

#define optional_entry(_oid, _name) \
	{ .oid = _oid, .type = CTLTYPE_INT, .exttype = "I", .name = #_name, .handler = handle_optional_##_name }

// XNU registers these with OID_AUTO, so the numbers here are arbitrary.
static const struct known_sysctl sysctls_hw_optional_arm[] = {
	optional_entry(1, FEAT_LSE),
	optional_entry(2, FEAT_LSE2),
	optional_entry(3, FEAT_CRC32),
	optional_entry(4, FEAT_AES),
	optional_entry(5, FEAT_PMULL),
	optional_entry(6, FEAT_SHA1),
	optional_entry(7, FEAT_SHA256),
	optional_entry(8, FEAT_SHA512),
	optional_entry(9, FEAT_SHA3),
	optional_entry(10, FEAT_FP16),
	optional_entry(11, FEAT_RDM),
	optional_entry(12, FEAT_JSCVT),
	optional_entry(13, FEAT_FCMA),
	optional_entry(14, FEAT_LRCPC),
	optional_entry(15, FEAT_LRCPC2),
	optional_entry(16, FEAT_DPB),
	optional_entry(17, FEAT_DotProd),
	optional_entry(18, FEAT_FHM),
	optional_entry(19, FEAT_DIT),
	optional_entry(20, FEAT_FlagM),
	optional_entry(21, FEAT_SSBS),
	optional_entry(22, FEAT_SB),
	optional_entry(23, FEAT_DPB2),
	optional_entry(24, FEAT_FlagM2),
	optional_entry(25, FEAT_FRINTTS),
	optional_entry(26, FEAT_I8MM),
	optional_entry(27, FEAT_BF16),
	optional_entry(28, FEAT_BTI),
	{ .oid = -1 }
};

static const struct known_sysctl sysctls_hw_optional[] = {
	optional_entry(1, floatingpoint),
	optional_entry(2, neon),
	optional_entry(3, neon_hpfp),
	optional_entry(4, neon_fp16),
	optional_entry(5, armv8_1_atomics),
	optional_entry(6, armv8_crc32),
	optional_entry(7, armv8_2_fhm),
	optional_entry(8, armv8_2_sha512),
	optional_entry(9, armv8_2_sha3),
	optional_entry(10, armv8_3_compnum),
	optional_entry(11, arm64),
	{ .oid = 12, .type = CTLTYPE_NODE, .exttype = "", .name = "arm", .subctls = sysctls_hw_optional_arm },
	{ .oid = -1 }
};
#endif

const struct known_sysctl sysctls_hw[] = {
	{ .oid = HW_NCPU, .type = CTLTYPE_INT, .exttype = "", .name = "ncpu", .handler = handle_ncpu },
	{ .oid = HW_AVAILCPU, .type = CTLTYPE_INT, .exttype = "", .name = "activecpu", .handler = handle_activecpu },
	// XNU registers HW_AVAILCPU only as "activecpu"; this older Darling name is still queried by Darling's Foundation
	{ .oid = _HW_AVAILCPU_COMPAT, .type = CTLTYPE_INT, .exttype = "", .name = "availcpu", .handler = handle_activecpu },
	{ .oid = _HW_PHYSICAL_CPU, .type = CTLTYPE_INT, .exttype = "", .name = "physicalcpu", .handler = handle_physicalcpu },
	{ .oid = _HW_PHYSICAL_CPU_MAX, .type = CTLTYPE_INT, .exttype = "", .name = "physicalcpu_max", .handler = handle_physicalcpu_max },
	{ .oid = _HW_LOGICAL_CPU, .type = CTLTYPE_INT, .exttype = "", .name = "logicalcpu", .handler = handle_logicalcpu },
	{ .oid = _HW_LOGICAL_CPU_MAX, .type = CTLTYPE_INT, .exttype = "", .name = "logicalcpu_max", .handler = handle_logicalcpu_max },
	{ .oid = HW_MEMSIZE, .type = CTLTYPE_QUAD, .exttype = "U", .name = "memsize", .handler = handle_memsize },
	{ .oid = HW_PAGESIZE, .type = CTLTYPE_INT, .exttype = "I", .name = "pagesize", .handler = handle_pagesize },
	{ .oid = _HW_CPUTYPE, .type = CTLTYPE_INT, .exttype = "", .name = "cputype", .handler = handle_cputype },
	{ .oid = _HW_CPUSUBTYPE, .type = CTLTYPE_INT, .exttype = "", .name = "cpusubtype", .handler = handle_cpusubtype },
	{ .oid = _HW_CPUTHREADTYPE, .type = CTLTYPE_INT, .exttype = "", .name = "cputhreadtype", .handler = handle_cputhreadtype },
	{ .oid = _HW_64BITCAPABLE, .type = CTLTYPE_INT, .exttype = "", .name = "cpu64bit_capable", .handler = handle_cpu64bitcapable },
	{ .oid = HW_MACHINE, .type = CTLTYPE_STRING, .exttype = "S", .name = "machine", .handler = handle_machine },
	{ .oid = HW_MODEL, .type = CTLTYPE_STRING, .exttype = "S", .name = "model", .handler = handle_model },
	{ .oid = _HW_CPUFREQUENCY, .type = CTLTYPE_QUAD, .exttype = "Q", .name = "cpufrequency", .handler = handle_cpufrequency },
	{ .oid = HW_CPU_FREQ, .type = CTLTYPE_INT, .exttype = "I", .name = "cpufrequency_compat", .handler = handle_cpufrequency_compat },
	{ .oid = HW_CACHELINE, .type = CTLTYPE_QUAD, .exttype = "U", .name = "cachelinesize", .handler = handle_cachelinesize },
	{ .oid = _HW_CACHECONFIG, .type = CTLTYPE_OPAQUE, .exttype = "Q", .name = "cacheconfig", .handler = handle_cacheconfig },
	{ .oid = _HW_CACHESIZE, .type = CTLTYPE_OPAQUE, .exttype = "Q", .name = "cachesize", .handler = handle_cachesize },
	{ .oid = HW_L1ICACHESIZE, .type = CTLTYPE_QUAD, .exttype = "Q", .name = "l1icachesize", .handler = handle_l1icachesize },
	{ .oid = HW_L1DCACHESIZE, .type = CTLTYPE_QUAD, .exttype = "Q", .name = "l1dcachesize", .handler = handle_l1dcachesize },
	{ .oid = HW_L2CACHESIZE, .type = CTLTYPE_QUAD, .exttype = "Q", .name = "l2cachesize", .handler = handle_l2cachesize },
	{ .oid = HW_L3CACHESIZE, .type = CTLTYPE_QUAD, .exttype = "Q", .name = "l3cachesize", .handler = handle_l3cachesize },
#if defined(__i386__) || defined(__x86_64__) || defined(__aarch64__)
	{ .oid = _HW_OPTIONAL, .type = CTLTYPE_NODE, .exttype = "", .name = "optional", .subctls = sysctls_hw_optional },
#endif
	{ .oid = -1 }
};

static const struct host_basic_info* gethostinfo(void)
{
	static struct host_basic_info hinfo;

	if (!hinfo.avail_cpus)
	{
		mach_msg_type_number_t hcount = HOST_BASIC_INFO_COUNT;
		mach_port_t hself;

		hself = mach_host_self();
		host_info(hself, HOST_BASIC_INFO, &hinfo, &hcount);
		mach_port_deallocate(mach_task_self(), hself);
	}

	return &hinfo;
}

// the maximum number of CPUs that could be available this boot
sysctl_handler(handle_ncpu)
{
	sysctl_handle_size(sizeof(int));
	*((int*) old) = gethostinfo()->max_cpus;
	return 0;
}

// The number of CPUs in this process's Linux affinity mask, or 0 if it can't be read.
// Unlike the host-wide counts, this changes with sched_setaffinity, taskset and cpusets.
static int affinity_cpu_count(void)
{
	unsigned char mask[128]; // enough for 1024 CPUs
	long size = LINUX_SYSCALL(__NR_sched_getaffinity, 0, sizeof(mask), mask);
	int count = 0;

	if (size <= 0)
		return 0;

	for (long i = 0; i < size && i < (long) sizeof(mask); i++) {
		for (unsigned char bits = mask[i]; bits != 0; bits &= bits - 1)
			count++;
	}

	return count;
}

// the number of CPUs currently available for executing this process's threads
sysctl_handler(handle_activecpu)
{
	sysctl_handle_size(sizeof(int));

	int count = gethostinfo()->avail_cpus;
	int allowed = affinity_cpu_count();

	if (allowed > 0 && allowed < count)
		count = allowed;

	*((int*) old) = count;
	return 0;
}

sysctl_handler(handle_physicalcpu)
{
	sysctl_handle_size(sizeof(int));
	*((int*) old) = gethostinfo()->physical_cpu;
	return 0;
}

sysctl_handler(handle_physicalcpu_max)
{
	sysctl_handle_size(sizeof(int));
	*((int*) old) = gethostinfo()->physical_cpu_max;
	return 0;
}

sysctl_handler(handle_logicalcpu)
{
	sysctl_handle_size(sizeof(int));
	*((int*) old) = gethostinfo()->logical_cpu;
	return 0;
}

sysctl_handler(handle_logicalcpu_max)
{
	sysctl_handle_size(sizeof(int));
	*((int*) old) = gethostinfo()->logical_cpu_max;
	return 0;
}

sysctl_handler(handle_memsize)
{

	sysctl_handle_size(sizeof(unsigned long long));
	*((unsigned long long*) old) = gethostinfo()->max_mem;
	return 0;
}

sysctl_handler(handle_pagesize)
{
	if (old == NULL)
	{
		*oldlen = sizeof(int);
		return 0;
	}
	else if (*oldlen < sizeof(int))
	{
		return -EINVAL;
	}
	else
	{
		*((int*) old) = 4096; // true on all Darling platforms

		// libc's sysconf passes in a `long` for the argument.
		// Apple's code is actually wrong there because it doesn't check the returned size.
		// to deal with this (since apparently it works on real macOS), what we do is zero out
		// the rest of the buffer so the upper half won't be garbage.
		//
		// note that this only works on little endian systems,
		// but that's okay; that's what all Darling platforms are.
		memset((char*)old + sizeof(int), 0, *oldlen - sizeof(int));

		*oldlen = sizeof(int);
	}
	return 0;
}

sysctl_handler(handle_cputype)
{
	sysctl_handle_size(sizeof(int));
	*((int*) old) = gethostinfo()->cpu_type;
	return 0;
}

sysctl_handler(handle_cpusubtype)
{
	sysctl_handle_size(sizeof(int));
	*((int*) old) = gethostinfo()->cpu_subtype;
	return 0;
}

sysctl_handler(handle_cputhreadtype)
{
	sysctl_handle_size(sizeof(int));
	*((int*) old) = gethostinfo()->cpu_threadtype;
	return 0;
}

// Darwin reports hertz; Linux cpufreq reports kilohertz.
static uint64_t cpu_frequency_hz(void)
{
	uint64_t khz = 2400000;
	char buf[16];

	// The prefix has no /sys of its own; the host's is under /Volumes/SystemRoot.
	if (read_string("/Volumes/SystemRoot/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq", buf, sizeof(buf)))
		khz = __simple_atoi(buf, NULL);

	return khz * 1000;
}

sysctl_handler(handle_cpufrequency)
{
	sysctl_handle_size(sizeof(uint64_t));
	*((uint64_t*) old) = cpu_frequency_hz();
	return 0;
}

// HW_CPU_FREQ keeps the 32-bit ABI and saturates, as XNU's cpu_clock_rate_hz does.
sysctl_handler(handle_cpufrequency_compat)
{
	uint64_t hz;

	sysctl_handle_size(sizeof(uint32_t));
	hz = cpu_frequency_hz();
	*((uint32_t*) old) = hz > UINT32_MAX ? UINT32_MAX : (uint32_t) hz;
	return 0;
}

sysctl_handler(handle_cpu64bitcapable)
{
	sysctl_handle_size(sizeof(int));
	*((int*) old) = 1; // Darling only supports 64-bit CPUs
	return 0;
}

sysctl_handler(handle_machine)
{
#if defined(__aarch64__)
	// Linux calls this architecture "aarch64"; Darwin (and thus uname -m) calls it "arm64".
	return copyout_string("arm64", (char*) old, oldlen);
#else
	return copyout_string(need_uname()->machine, (char*) old, oldlen);
#endif
}

sysctl_handler(handle_cachelinesize)
{
	// A fixed architectural constant, like the page size above. XNU aligns its cache
	// maintenance on 1 << ARM64_CLINE_SHIFT with ARM64_CLINE_SHIFT == 6
	// (osfmk/arm64/proc_reg.h), i.e. 64 bytes on arm64; on arm, MMU_CLINE is 6 for
	// the same reason (osfmk/arm/proc_reg.h).
	sysctl_handle_size(sizeof(unsigned long long));
	*((unsigned long long*) old) = 64;
	return 0;
}

// hw.cacheconfig and hw.cachesize are arrays of this many 64-bit values,
// indexed by cache level; index 0 describes memory.
#define CACHE_LEVELS 10

// The prefix has no /sys of its own; the host's is under /Volumes/SystemRoot.
#define HOST_CACHE_DIR "/Volumes/SystemRoot/sys/devices/system/cpu/cpu0/cache/"

static unsigned long long parse_number(const char* str, const char** end)
{
	unsigned long long value = 0;

	while (*str >= '0' && *str <= '9')
		value = value * 10 + (*str++ - '0');
	if (end != NULL)
		*end = str;
	return value;
}

// Counts the CPUs in a Linux CPU list such as "0-3,8-11".
static unsigned int count_cpu_list(const char* list)
{
	unsigned int count = 0;

	while (*list >= '0' && *list <= '9')
	{
		unsigned long long first = parse_number(list, &list), last = first;

		if (*list == '-')
			last = parse_number(list + 1, &list);
		if (last >= first)
			count += last - first + 1;
		if (*list != ',')
			break;
		list++;
	}
	return count;
}

// Looks up the cache of the given level (1 = L1) on the host's first CPU: the
// instruction cache if instruction is set, otherwise the data or unified one.
static bool host_cache_level(unsigned int level, bool instruction, unsigned long long* size, unsigned int* sharing)
{
	char path[128], buf[64];

	for (int index = 0; index < CACHE_LEVELS; index++)
	{
		const char* end;

		__simple_sprintf(path, HOST_CACHE_DIR "index%d/level", index);
		if (!read_string(path, buf, sizeof(buf)))
			break;
		if (parse_number(buf, NULL) != level)
			continue;

		__simple_sprintf(path, HOST_CACHE_DIR "index%d/type", index);
		if (!read_string(path, buf, sizeof(buf)) || (strncmp(buf, "Instruction", 11) == 0) != instruction)
			continue;

		__simple_sprintf(path, HOST_CACHE_DIR "index%d/size", index);
		if (!read_string(path, buf, sizeof(buf)))
			continue;
		*size = parse_number(buf, &end);
		if (*end == 'K')
			*size <<= 10;
		else if (*end == 'M')
			*size <<= 20;

		__simple_sprintf(path, HOST_CACHE_DIR "index%d/shared_cpu_list", index);
		*sharing = read_string(path, buf, sizeof(buf)) ? count_cpu_list(buf) : 0;
		if (*sharing == 0)
			*sharing = 1;
		return true;
	}
	return false;
}

// [0] is the number of CPUs sharing memory, [n] the number sharing a level n cache.
sysctl_handler(handle_cacheconfig)
{
	unsigned long long* config = (unsigned long long*) old;

	sysctl_handle_size(CACHE_LEVELS * sizeof(*config));
	memset(config, 0, CACHE_LEVELS * sizeof(*config));
	config[0] = gethostinfo()->logical_cpu_max;
	for (unsigned int level = 1; level < CACHE_LEVELS; level++)
	{
		unsigned long long size;
		unsigned int sharing;

		if (!host_cache_level(level, false, &size, &sharing))
			break;
		config[level] = sharing;
	}
	return 0;
}

// [0] is the memory size, [n] the size of the level n cache in bytes.
sysctl_handler(handle_cachesize)
{
	unsigned long long* sizes = (unsigned long long*) old;

	sysctl_handle_size(CACHE_LEVELS * sizeof(*sizes));
	memset(sizes, 0, CACHE_LEVELS * sizeof(*sizes));
	sizes[0] = gethostinfo()->max_mem;
	for (unsigned int level = 1; level < CACHE_LEVELS; level++)
	{
		unsigned long long size;
		unsigned int sharing;

		if (!host_cache_level(level, false, &size, &sharing))
			break;
		sizes[level] = size;
	}
	return 0;
}

// hw.l*cachesize are absent, as on macOS, when the host has no such cache.
static long copyout_cache_size(unsigned int level, bool instruction, void* old, unsigned long* oldlen)
{
	unsigned long long size;
	unsigned int sharing;

	if (!host_cache_level(level, instruction, &size, &sharing))
		return -ENOENT;
	sysctl_handle_size(sizeof(size));
	*((unsigned long long*) old) = size;
	return 0;
}

sysctl_handler(handle_l1icachesize)
{
	return copyout_cache_size(1, true, old, oldlen);
}

sysctl_handler(handle_l1dcachesize)
{
	return copyout_cache_size(1, false, old, oldlen);
}

sysctl_handler(handle_l2cachesize)
{
	return copyout_cache_size(2, false, old, oldlen);
}

sysctl_handler(handle_l3cachesize)
{
	return copyout_cache_size(3, false, old, oldlen);
}
