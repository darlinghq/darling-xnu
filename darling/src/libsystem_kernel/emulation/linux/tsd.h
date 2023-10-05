#ifndef _DARLING_EMULATION_TSD_H_
#define _DARLING_EMULATION_TSD_H_

#include <pthread/tsd_private.h>

#define __PTK_DARLING_XTRACE_TLS          __PTK_DARLING_KEY0
#define __PTK_DARLING_PERTHREAD_WD_IS_SET __PTK_DARLING_KEY1
#define __PTK_DARLING_PERTHREAD_WD_FD     __PTK_DARLING_KEY2
// other __PTK_DARLING_KEYs are currently unused

#endif // _DARLING_EMULATION_TSD_H_
