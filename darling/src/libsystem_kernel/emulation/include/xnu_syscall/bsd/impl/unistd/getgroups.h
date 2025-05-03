#ifndef LINUX_GETGROUPS_H
#define LINUX_GETGROUPS_H

long __getgroups(unsigned int size, unsigned int* gidset);
long __setgroups(unsigned int size, const unsigned int* gidset);

long sys_getgroups(unsigned int size, int* gidset);

#endif // LINUX_GETGROUPS_H
