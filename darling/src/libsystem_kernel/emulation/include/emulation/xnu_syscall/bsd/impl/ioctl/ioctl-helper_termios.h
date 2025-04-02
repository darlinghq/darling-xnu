#ifndef DARLING_BSD_EMULATION_IOCTL_HELPER_TERMIOS_H
#define DARLING_BSD_EMULATION_IOCTL_HELPER_TERMIOS_H

int handle_termios(int fd, unsigned int cmd, void* arg, int* retval);

// I'm not sure where TIOCISATTY comes from...
#define BSD_TIOCISATTY      0x2000745E

#endif // DARLING_BSD_EMULATION_IOCTL_HELPER_TERMIOS_H
