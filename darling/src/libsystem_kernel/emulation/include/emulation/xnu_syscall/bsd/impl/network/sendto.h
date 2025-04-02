#ifndef DARLING_BSD_EMULATION_SENDTO_H
#define DARLING_BSD_EMULATION_SENDTO_H

long sys_sendto(int fd, const void* buffer, unsigned long length,
		int flags, const void* sockaddr, int socklen);
long sys_sendto_nocancel(int fd, const void* buffer, unsigned long length,
		int flags, const void* sockaddr, int socklen);

// TODO: Move into conversion

#define LINUX_SYS_SENDTO	11

#endif // DARLING_BSD_EMULATION_SENDTO_H
