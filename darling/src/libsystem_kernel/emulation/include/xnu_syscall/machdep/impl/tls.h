#ifndef LINUX_TLS_H
#define LINUX_TLS_H

#include <darling/emulation/common/base.h>
#include <stdbool.h>

void sys_thread_set_tsd_base(void* ptr, int unk);

#if defined(__aarch64__)
#include <stdint.h>
void* sys_thread_get_tsd_base(void);
bool sys_thread_has_tsd_base(void);
unsigned long sys_thread_get_native_tsd_slot_offset(void);
void sys_thread_set_native_tsd_slot_offset(unsigned long offset);
intptr_t sys_thread_get_rseq_cpu_id_offset(void);
void sys_thread_set_rseq_cpu_offset(intptr_t cpu_id_start_offset);
int sys_thread_rseq_cpu_number(void);
void sys_thread_rseq_cleanup(void);
void sys_thread_rseq_postfork_child(void);
#endif

#endif // LINUX_TLS_H
