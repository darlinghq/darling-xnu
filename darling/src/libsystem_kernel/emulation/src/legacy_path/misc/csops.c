#include "csops.h"
#include <stdint.h>
#include <sys/codesign.h>
#include "../duct_errno.h"
#include <strings.h>

// TODO: actually implement this
// this is just a dummy implementation for now that returns no actual data for any operation
long sys_csops(int pid, unsigned int ops, void* useraddr, size_t usersize) {
	int ret = 0;

	switch (ops) {
		case CS_OPS_STATUS: {
			*(uint32_t*)useraddr = CS_VALID;
		} break;

		case CS_OPS_BLOB:
		case CS_OPS_ENTITLEMENTS_BLOB:
		case CS_OPS_IDENTITY: {
			memset(useraddr, 0, usersize);
		} break;

		// all other operations are completely unsupported
		default: {
			ret = -ENOSYS;
		} break;
	}

	return ret;
};

// also more or less a stub
long sys_csops_audittoken(int pid, unsigned int ops, void* useraddr, size_t usersize, audit_token_t* audit_token) {
	// just defer to the regular csops
	return sys_csops(pid, ops, useraddr, usersize);
};
