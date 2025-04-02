#ifndef DARLING_EMULATION_CONVERSION_SIGNAL_H
#define DARLING_EMULATION_CONVERSION_SIGNAL_H

#include <emulation/conversion/variables/bsd/sys/signal.h>
#include <emulation/conversion/variables/linux/asm/signal.h>

int signum_linux_to_bsd(int signum);
int signum_bsd_to_linux(int signum);
void sigset_linux_to_bsd(const linux_sigset_t* linux, bsd_sigset_t* bsd);
void sigset_bsd_to_linux(const bsd_sigset_t* bsd, linux_sigset_t* linux);

#endif // DARLING_EMULATION_CONVERSION_SIGNAL_H
