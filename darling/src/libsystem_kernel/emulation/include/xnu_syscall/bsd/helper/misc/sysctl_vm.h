#ifndef _DARLING_EMULATION_LINUX_MISC_SYSCTL_VM_H_
#define _DARLING_EMULATION_LINUX_MISC_SYSCTL_VM_H_

#include <darling/emulation/xnu_syscall/bsd/impl/misc/sysctl.h>

extern const struct known_sysctl sysctls_vm[];
int _sysctl_memorystatus_level(int* level);

#endif // _DARLING_EMULATION_LINUX_MISC_SYSCTL_VM_H_
