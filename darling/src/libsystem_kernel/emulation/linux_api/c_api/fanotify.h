#ifndef DARLING_EMULATION_LINUX_API_FANOTIFY_H
#define DARLING_EMULATION_LINUX_API_FANOTIFY_H

#include <darling/emulation/common/base.h>

CPP_EXTERN_C_BEGIN

VISIBLE
int linux_fanotify_init(unsigned flags, unsigned event_f_flags);

VISIBLE
int linux_fanotify_mark(int fanotify_fd, unsigned flags, unsigned long long mask, int dfd, const char *pathname);

CPP_EXTERN_C_END

#endif // DARLING_EMULATION_LINUX_API_FANOTIFY_H
