#ifndef DARLING_EMULATION_CONVERSION_BSD_I386__PARAM_H
#define DARLING_EMULATION_CONVERSION_BSD_I386__PARAM_H

#include <emulation/conversion/variables/bsd/i386/_types.h>

#define BSD___DARWIN_ALIGNBYTES32     (sizeof(bsd___uint32_t) - 1)
#define BSD___DARWIN_ALIGN32(p)       ((bsd___darwin_size_t)((bsd___darwin_size_t)(p) + BSD___DARWIN_ALIGNBYTES32) &~ BSD___DARWIN_ALIGNBYTES32)

#endif // DARLING_EMULATION_CONVERSION_BSD_I386__PARAM_H
