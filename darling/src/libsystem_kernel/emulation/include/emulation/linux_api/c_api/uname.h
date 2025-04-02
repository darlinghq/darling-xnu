#ifndef DARLING_EMULATION_CONVERSION_UTSNAME_H
#define DARLING_EMULATION_CONVERSION_UTSNAME_H

#include <emulation/conversion/variables/linux/linux/utsname.h>

int __linux_uname(struct linux_new_utsname* p);

#endif // DARLING_EMULATION_CONVERSION_UTSNAME_H
