#ifndef DARLING_EMULATION_CONVERSION_BSD_SYS__TYPES__INO_T_H
#define DARLING_EMULATION_CONVERSION_BSD_SYS__TYPES__INO_T_H

#include <emulation/conversion/variables/bsd/sys/_types.h>

// Because the type of `bsd_ino_t` can vary, we will instead provide an 
// explicit version that represents the 32bit version.
typedef bsd___darwin_ino32_t          bsd_ino32_t;

#endif // DARLING_EMULATION_CONVERSION_BSD_SYS__TYPES__INO_T_H