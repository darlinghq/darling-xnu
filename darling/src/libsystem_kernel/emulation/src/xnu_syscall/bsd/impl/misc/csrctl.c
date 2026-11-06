#include <darling/emulation/xnu_syscall/bsd/impl/misc/csrctl.h>

#define PRIVATE
#include <sys/csr.h>
#include <sys/errno.h>
#include <stddef.h>

#include <darling/emulation/conversion/duct_errno.h>

long sys_csrctl(uint32_t op, void* useraddr, size_t usersize) {
	switch (op) {
		case CSR_SYSCALL_CHECK: {
			if (useraddr == NULL || usersize != sizeof(uint32_t))
				return -EINVAL;
			// allow everything
			return 0;
		} break;
		case CSR_SYSCALL_GET_ACTIVE_CONFIG: {
			if (useraddr == NULL || usersize != sizeof(uint32_t))
				return -EINVAL;
			// set everything to "allowed"
			*(uint32_t*)useraddr = CSR_VALID_FLAGS;
			return 0;
		} break;
		default:
			return -ENOSYS;
	}
}
