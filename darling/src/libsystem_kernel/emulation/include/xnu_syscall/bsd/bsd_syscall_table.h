#ifndef LINUX_SYSCALLS_H
#define LINUX_SYSCALLS_H

// Shared with the i386 dispatcher, which bounds the syscall number with it.
#define BSD_SYSCALL_TABLE_SIZE 600

#ifndef __ASSEMBLER__
extern void* __bsd_syscall_table[BSD_SYSCALL_TABLE_SIZE];
#endif

#endif // LINUX_SYSCALLS_H
