#ifndef DUCT_MMAN_H
#define DUCT_MMAN_H

#define LINUX_PROT_READ 0x1
#define LINUX_PROT_WRITE 0x2
#define LINUX_PROT_EXEC 0x4
#define LINUX_PROT_SEM 0x8
#define LINUX_PROT_NONE 0x0
#define LINUX_PROT_GROWSDOWN 0x01000000
#define LINUX_PROT_GROWSUP 0x02000000
#define LINUX_MAP_SHARED 0x01
#define LINUX_MAP_PRIVATE 0x02
#define LINUX_MAP_TYPE 0x0f
#define LINUX_MAP_FIXED 0x10
#define LINUX_MAP_ANONYMOUS 0x20
#define LINUX_MS_ASYNC 1
#define LINUX_MS_INVALIDATE 2
#define LINUX_MS_SYNC 4
#define LINUX_MADV_NORMAL 0
#define LINUX_MADV_RANDOM 1
#define LINUX_MADV_SEQUENTIAL 2
#define LINUX_MADV_WILLNEED 3
#define LINUX_MADV_DONTNEED 4
#define LINUX_MADV_REMOVE 9
#define LINUX_MADV_DONTFORK 10
#define LINUX_MADV_DOFORK 11
#define LINUX_MADV_HWPOISON 100
#define LINUX_MADV_SOFT_OFFLINE 101
#define LINUX_MADV_MERGEABLE 12
#define LINUX_MADV_UNMERGEABLE 13
#define LINUX_MADV_HUGEPAGE 14
#define LINUX_MADV_NOHUGEPAGE 15
#define LINUX_MADV_DONTDUMP 16
#define LINUX_MADV_DODUMP 17
#define LINUX_MAP_FILE 0
#define LINUX_MAP_HUGE_SHIFT 26
#define LINUX_MAP_HUGE_MASK 0x3f
#define LINUX_MAP_GROWSDOWN 0x0100
#define LINUX_MAP_DENYWRITE 0x0800
#define LINUX_MAP_EXECUTABLE 0x1000
#define LINUX_MAP_LOCKED 0x2000
#define LINUX_MAP_NORESERVE 0x4000
#define LINUX_MAP_POPULATE 0x8000
#define LINUX_MAP_NONBLOCK 0x10000
#define LINUX_MAP_STACK 0x20000
#define LINUX_MAP_HUGETLB 0x40000
#define LINUX_MAP_32BIT 0x40
#define LINUX_MCL_CURRENT 1
#define LINUX_MCL_FUTURE 2

#include <sys/mman.h>

#endif // DUCT_MMAN_H
