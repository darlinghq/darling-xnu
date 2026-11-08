#include <darling/emulation/xnu_syscall/bsd/impl/misc/csrctl.h>

#define PRIVATE
#include <sys/csr.h>
#include <sys/errno.h>
#include <stddef.h>

#include <darling/emulation/conversion/errno.h>

#include <darlingserver/rpc.h>
#include <darlingserver/rpc-supplement.h>

// System Integrity Protection is configured with the `csr-active-config`
// value darlingserver holds (the `org.darlinghq.security` domain; by default
// what `csrutil disable` sets). Darling only reports it: nothing here
// protects any file.
static int active_config(uint32_t* config) {
	dserver_system_config_t system_config;
	int ret = dserver_rpc_get_system_config(&system_config, sizeof(system_config));
	if (ret < 0)
		return errno_linux_to_bsd(ret);
	*config = system_config.csr_active_config;
	return 0;
}

long sys_csrctl(uint32_t op, void* useraddr, size_t usersize) {
	uint32_t config;
	int ret;

	switch (op) {
		case CSR_SYSCALL_CHECK: {
			uint32_t mask;
			if (useraddr == NULL || usersize != sizeof(uint32_t))
				return -EINVAL;
			mask = *(uint32_t*)useraddr;
			ret = active_config(&config);
			if (ret != 0)
				return ret;
			// with SIP off (untrusted kexts allowed), the kernel debugger
			// is allowed too
			if (config & (CSR_ALLOW_UNTRUSTED_KEXTS | CSR_ALLOW_APPLE_INTERNAL))
				config |= CSR_ALLOW_KERNEL_DEBUGGER;
			return ((config & mask) == mask) ? 0 : -EPERM;
		} break;
		case CSR_SYSCALL_GET_ACTIVE_CONFIG: {
			if (useraddr == NULL || usersize != sizeof(uint32_t))
				return -EINVAL;
			ret = active_config(&config);
			if (ret != 0)
				return ret;
			*(uint32_t*)useraddr = config;
			return 0;
		} break;
		default:
			return -ENOSYS;
	}
}
