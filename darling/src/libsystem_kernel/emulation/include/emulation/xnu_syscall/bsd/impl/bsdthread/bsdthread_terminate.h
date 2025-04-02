#ifndef DARLING_BSD_EMULATION_BSDTHREAD_TERMINATE_H
#define DARLING_BSD_EMULATION_BSDTHREAD_TERMINATE_H

long sys_bsdthread_terminate(void* stackaddr, unsigned long freesize, int port, int join_sem);

#endif // DARLING_BSD_EMULATION_BSDTHREAD_TERMINATE_H
