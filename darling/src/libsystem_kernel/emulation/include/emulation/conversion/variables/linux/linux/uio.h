#ifndef DARLING_EMULATION_CONVERSION_LINUX_LINUX_UIO_H
#define DARLING_EMULATION_CONVERSION_LINUX_LINUX_UIO_H

#include <emulation/conversion/variables/linux/linux/types.h>

struct linux_iovec
{
	void                    *iov_base;
	linux___kernel_size_t   iov_len;
};

#endif // DARLING_EMULATION_CONVERSION_LINUX_LINUX_UIO_H
