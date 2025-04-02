#ifndef DARLING_BSD_EMULATION_SEM_WAIT_H
#define DARLING_BSD_EMULATION_SEM_WAIT_H

long sys_sem_wait(int* sem);
long sys_sem_wait_nocancel(int* sem);

#endif // DARLING_BSD_EMULATION_SEM_WAIT_H
