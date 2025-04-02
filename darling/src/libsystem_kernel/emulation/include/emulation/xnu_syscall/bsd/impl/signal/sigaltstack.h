#ifndef DARLING_BSD_EMULATION_SIGALTSTACK_H
#define DARLING_BSD_EMULATION_SIGALTSTACK_H

#include <emulation/conversion/variables/bsd/sys/_types/_sigaltstack.h>

long sys_sigaltstack(const bsd_stack_t* ss, bsd_stack_t* oss);

#endif // DARLING_BSD_EMULATION_SIGALTSTACK_H
