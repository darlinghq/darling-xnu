#ifndef LINUX_TLS_H
#define LINUX_TLS_H

#include <darling/emulation/common/base.h>
#include <stdbool.h>

void sys_thread_set_tsd_base(void* ptr, int unk);

#if defined(__aarch64__)
void* sys_thread_get_tsd_base(void);
bool sys_thread_has_tsd_base(void);
#endif

#endif // LINUX_TLS_H
