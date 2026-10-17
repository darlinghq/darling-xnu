#ifndef LINUX_MREMAP_ENCRYPTED_H
#define LINUX_MREMAP_ENCRYPTED_H

long sys_mremap_encrypted(void* addr, unsigned long len, unsigned int cryptid, unsigned int cputype, unsigned int cpusubtype);

#endif
