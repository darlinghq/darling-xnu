#ifndef DARLING_MLOCKALL_H
#define DARLING_MLOCKALL_H

long sys_mlockall(int how);
long sys_munlockall(int how);

#endif
