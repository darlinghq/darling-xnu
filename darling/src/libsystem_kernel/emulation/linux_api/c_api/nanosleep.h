#ifndef DARLING_EMULATION_LINUX_API_NANOSLEEP_H
#define DARLING_EMULATION_LINUX_API_NANOSLEEP_H

extern int __linux_nanosleep(struct timespec* ts, struct timespec* rem);

#endif // DARLING_EMULATION_LINUX_API_NANOSLEEP_H