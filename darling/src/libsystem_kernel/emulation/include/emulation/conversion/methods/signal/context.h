#ifndef DARLING_EMULATION_CONVERSION_CONTEXT_H
#define DARLING_EMULATION_CONVERSION_CONTEXT_H

// #include <_libkernel_init.h>

#include <emulation/conversion/methods/signal.h>
#include <emulation/conversion/variables/bsd/mach/machine/thread_status.h>
#include <emulation/conversion/variables/bsd/machine/_mcontext.h>
#include <emulation/conversion/variables/bsd/sys/_types/_ucontext.h>
#include <emulation/conversion/variables/linux/asm/signal.h>
#include <emulation/conversion/variables/linux/asm/sigcontext.h>
#include <emulation/xnu_syscall/bsd/impl/signal/sigaltstack.h>

// The following must be defined before `linux/asm-generic/ucontext.h`:
// * `linux/asm/signal.h`
// * `linux/asm/sigcontext.h`
#include <emulation/conversion/variables/linux/asm-generic/ucontext.h> 


void ucontext_linux_to_bsd(const struct linux_ucontext* lc, struct bsd_ucontext* bc, struct bsd_mcontext *bm);
void mcontext_bsd_to_linux(const struct bsd_mcontext *bm, struct linux_sigcontext* lm);

#if defined(__x86_64__)
void mcontext_to_thread_state(const struct linux_sigcontext* sigcontext, bsd_x86_thread_state64_t* s);
void mcontext_to_float_state(const struct linux_sigcontext* sigcontext, bsd_x86_float_state64_t* s);
void thread_state_to_mcontext(const bsd_x86_thread_state64_t* s, struct linux_sigcontext* sigcontext);
void float_state_to_mcontext(const bsd_x86_float_state64_t* s, struct linux_sigcontext* sigcontext);
#elif defined(__i386__)
void mcontext_to_thread_state(const struct linux_sigcontext* sigcontext, bsd_x86_thread_state32_t* s);
void mcontext_to_float_state(const struct linux_sigcontext* sigcontext, bsd_x86_float_state32_t* s);
void thread_state_to_mcontext(const bsd_x86_thread_state32_t* s, struct linux_sigcontext* sigcontext);
void float_state_to_mcontext(const bsd_x86_float_state32_t* s, struct linux_sigcontext* sigcontext);
#endif

#endif // DARLING_EMULATION_CONVERSION_CONTEXT_H