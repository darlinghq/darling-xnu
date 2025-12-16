#ifndef LINUX_CLONEFILE_H
#define LINUX_CLONEFILE_H

#include <stdint.h>

#define XNU_CLONE_NOFOLLOW        0x0001
#define XNU_CLONE_NOOWNERCOPY     0x0002
#define XNU_CLONE_ACL             0x0004
#define XNU_CLONE_NOFOLLOW_ANY    0x0008
#define XNU_CLONE_RESOLVE_BENEATH 0x0010

long sys_clonefileat(int src_fd, const char* src_path, int dest_fd, const char* dest_path, uint32_t flags);
long sys_fclonefileat(int src_fd, int dest_fd, const char* dest_path, uint32_t flags);

#endif // LINUX_CLONEFILE_H
