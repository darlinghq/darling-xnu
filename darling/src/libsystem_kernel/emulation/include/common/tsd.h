#ifndef _DARLING_EMULATION_TSD_H_
#define _DARLING_EMULATION_TSD_H_

#include <pthread/tsd_private.h>

#define __PTK_DARLING_XTRACE_TLS          __PTK_DARLING_KEY0
#define __PTK_DARLING_PERTHREAD_WD_IS_SET __PTK_DARLING_KEY1
#define __PTK_DARLING_PERTHREAD_WD_FD     __PTK_DARLING_KEY2
#define __PTK_DARLING_CANCEL_MARKED       __PTK_DARLING_KEY3
#define __PTK_DARLING_SIGEXC_SIGNAL       __PTK_DARLING_KEY4
#define __PTK_DARLING_WORKQ_FLAGS         __PTK_DARLING_KEY5 // flags this workqueue thread was started with
#define __PTK_DARLING_WORKQ_KEVENT        __PTK_DARLING_KEY6 // kevent delivery this workqueue thread is servicing
// other __PTK_DARLING_KEYs are currently unused

#endif // _DARLING_EMULATION_TSD_H_
