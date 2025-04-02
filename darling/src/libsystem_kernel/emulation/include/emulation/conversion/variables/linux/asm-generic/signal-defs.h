#ifndef DARLING_EMULATION_CONVERSION_LINUX_ASM_GENERIC_SIGNAL_DEFS_H
#define DARLING_EMULATION_CONVERSION_LINUX_ASM_GENERIC_SIGNAL_DEFS_H

#ifndef LINUX_SA_NOCLDSTOP
#define LINUX_SA_NOCLDSTOP    0x00000001
#endif
#ifndef LINUX_SA_NOCLDWAIT
#define LINUX_SA_NOCLDWAIT    0x00000002
#endif
#ifndef LINUX_SA_SIGINFO
#define LINUX_SA_SIGINFO      0x00000004
#endif

#ifndef LINUX_SA_ONSTACK
#define LINUX_SA_ONSTACK      0x08000000
#endif
#ifndef LINUX_SA_RESTART
#define LINUX_SA_RESTART      0x10000000
#endif
#ifndef LINUX_SA_NODEFER
#define LINUX_SA_NODEFER      0x40000000
#endif
#ifndef LINUX_SA_RESETHAND
#define LINUX_SA_RESETHAND    0x80000000
#endif

typedef void linux__signalfn_t(int);
typedef void linux__restorefn_t(void);

typedef linux__signalfn_t* __linux_sighandler_t;
typedef linux__restorefn_t* __linux_sigrestore_t;

#define LINUX_SIG_DFL	((__linux_sighandler_t)0)	/* default signal handling */
#define LINUX_SIG_IGN	((__linux_sighandler_t)1)	/* ignore signal */
#define LINUX_SIG_ERR	((__linux_sighandler_t)-1)	/* error return from signal */

#endif // DARLING_EMULATION_CONVERSION_LINUX_ASM_GENERIC_SIGNAL_DEFS_H
