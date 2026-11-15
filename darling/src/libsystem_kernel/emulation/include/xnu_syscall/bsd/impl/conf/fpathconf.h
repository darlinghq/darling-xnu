#ifndef LINUX_FPATHCONF_H
#define LINUX_FPATHCONF_H

long sys_fpathconf(int fd, int name);
// Answer for an object of the given st_mode (also used by sys_pathconf).
long pathconf_for_mode(unsigned int mode, int name);

#endif // LINUX_FPATHCONF_H
