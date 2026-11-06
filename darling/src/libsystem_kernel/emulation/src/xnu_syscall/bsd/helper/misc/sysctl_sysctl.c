#include <darling/emulation/xnu_syscall/bsd/helper/misc/sysctl_sysctl.h>

#include <mach/machine.h>
#include <sys/errno.h>

#include <darlingserver/rpc.h>

enum {
	_PROC_CPUTYPE = 1000,
	_PROC_TRANSLATED,
};

static sysctl_handler(handle_proc_cputype);
static sysctl_handler(handle_proc_translated);

const struct known_sysctl sysctls_sysctl[] = {
	{ .oid = _PROC_CPUTYPE, .type = CTLTYPE_INT, .exttype = "I", .name = "proc_cputype", .handler = handle_proc_cputype },
	{ .oid = _PROC_TRANSLATED, .type = CTLTYPE_INT, .exttype = "I", .name = "proc_translated", .handler = handle_proc_translated },
	{ .oid = -1 }
};

sysctl_handler(handle_proc_cputype)
{
	if (nlen < 3)
		return -ENOENT;

	bool is64bit;
	if (dserver_rpc_task_is_64_bit(name[2], &is64bit) < 0) {
		return -ENOENT;
	}

	if (*oldlen < sizeof(int))
		return -EINVAL;

#if defined(__i386__) || defined(__x86_64__)
	*((unsigned int*) old) = CPU_TYPE_I386;
#elif defined(__arm__) || defined(__aarch64__)
	*((unsigned int*) old) = CPU_TYPE_ARM;
#else
#	warning Missing code for this arch!
	*((unsigned int*) old) = 0;
#endif

	if (is64bit)
		*((unsigned int*) old) |= CPU_ARCH_ABI64;


	return 0;
}

sysctl_handler(handle_proc_translated)
{
	// Code runs natively on the host CPU, never under binary translation
	sysctl_handle_size(sizeof(int));
	*((int*) old) = 0;
	return 0;
}
