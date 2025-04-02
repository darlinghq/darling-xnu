#ifndef DARLING_BSD_EMULATION_FCHMODAT_H
#define DARLING_BSD_EMULATION_FCHMODAT_H

long sys_fchmodat(int fd, const char* path, int mode, int flag);

#endif // DARLING_BSD_EMULATION_FCHMODAT_H
