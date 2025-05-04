#ifndef _EXT_FANOTIFY_H
#define _EXT_FANOTIFY_H

#include <darling/emulation/common/base.h>

CPP_EXTERN_BEGIN

VISIBLE
int fanotify_init(unsigned flags, unsigned event_f_flags);

VISIBLE
int fanotify_mark(int fanotify_fd, unsigned flags, unsigned long long mask, int dfd, const char *pathname);

CPP_EXTERN_END

#endif // _EXT_FANOTIFY_H
