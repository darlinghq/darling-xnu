#ifndef DARLING_EMULATION_CONVERSION_BSD_SYS__TYPES__IOVEC_T_H
#define DARLING_EMULATION_CONVERSION_BSD_SYS__TYPES__IOVEC_T_H

#include <emulation/conversion/variables/bsd/sys/_types/_size_t.h>

struct bsd_iovec {
	void *      iov_base;
	bsd_size_t  iov_len;
};

#endif // DARLING_EMULATION_CONVERSION_BSD_SYS__TYPES__IOVEC_T_H