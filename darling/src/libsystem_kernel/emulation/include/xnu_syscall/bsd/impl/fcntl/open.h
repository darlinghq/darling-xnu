#ifndef LINUX_OPEN_H
#define LINUX_OPEN_H

long sys_open(const char* filename, int flags, unsigned int mode);
long sys_open_nocancel(const char* filename, int flags, unsigned int mode);

#endif // LINUX_OPEN_H
