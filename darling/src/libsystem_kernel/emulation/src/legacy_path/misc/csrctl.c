#include <darling/emulation/legacy_path/misc/csrctl.h>

#define PRIVATE
#include <sys/csr.h>

#include <darling/emulation/legacy_path/duct_errno.h>

long sys_csrctl(uint32_t op, void* useraddr, size_t usersize) {
	switch (op) {
		case CSR_SYSCALL_CHECK: {
			// allow everything
			return 0;
		} break;
		case CSR_SYSCALL_GET_ACTIVE_CONFIG: {
			// set everything to "allowed"
			*(uint32_t*)useraddr = CSR_VALID_FLAGS;
			return 0;
		} break;
		default:
			return ENOSYS;
	}
};
