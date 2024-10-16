#ifndef DARLING_EMULATION_LINUX_API_MREMAP_H
#define DARLING_EMULATION_LINUX_API_MREMAP_H

long __linux_mremap(void* old_addr, unsigned long old_size, unsigned long new_size, int flags, void* new_address);

#endif // DARLING_EMULATION_LINUX_API_MREMAP_H

