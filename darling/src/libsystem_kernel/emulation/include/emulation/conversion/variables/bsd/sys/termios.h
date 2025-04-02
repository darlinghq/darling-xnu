#ifndef DARLING_EMULATION_CONVERSION_BSD_SYS_TERMIOS_H
#define DARLING_EMULATION_CONVERSION_BSD_SYS_TERMIOS_H

enum {
	BSD_VEOF     =  0,
	BSD_VEOL     =  1,
	BSD_VEOL2    =  2,
	BSD_VERASE   =  3,
	BSD_VWERASE  =  4,
	BSD_VKILL    =  5,
	BSD_VREPRINT =  6,
	/* spare = 7 */
	BSD_VINTR    =  8,
	BSD_VQUIT    =  9,
	BSD_VSUSP    = 10,
	BSD_VDSUSP   = 11,
	BSD_VSTART   = 12,
	BSD_VSTOP    = 13,
	BSD_VLNEXT   = 14,
	BSD_VDISCARD = 15,
	BSD_VMIN     = 16,
	BSD_VTIME    = 17,
	BSD_VSTATUS  = 18,
};

#define BSD_IGNBRK          0x00000001
#define BSD_BRKINT          0x00000002
#define BSD_IGNPAR          0x00000004
#define BSD_PARMRK          0x00000008
#define BSD_INPCK           0x00000010
#define BSD_ISTRIP          0x00000020
#define BSD_INLCR           0x00000040
#define BSD_IGNCR           0x00000080
#define BSD_ICRNL           0x00000100
#define BSD_IXON            0x00000200
#define BSD_IXOFF           0x00000400
#define BSD_IXANY           0x00000800
#define BSD_IMAXBEL         0x00002000
#define BSD_IUTF8           0x00004000

#define BSD_OPOST           0x00000001
#define BSD_ONLCR           0x00000002
#define BSD_OXTABS          0x00000004
#define BSD_ONOEOT          0x00000008

#define BSD_CIGNORE         0x00000001
#define BSD_CSIZE           0x00000300
#define     BSD_CS5             0x00000000
#define     BSD_CS6             0x00000100
#define     BSD_CS7             0x00000200
#define     BSD_CS8             0x00000300
#define BSD_CSTOPB          0x00000400
#define BSD_CREAD           0x00000800
#define BSD_PARENB          0x00001000
#define BSD_PARODD          0x00002000
#define BSD_HUPCL           0x00004000
#define BSD_CLOCAL          0x00008000
#define BSD_CCTS_OFLOW      0x00010000
#define BSD_CRTS_IFLOW      0x00020000
#define BSD_CDTR_IFLOW      0x00040000
#define BSD_CDSR_OFLOW      0x00080000
#define BSD_CCAR_OFLOW      0x00100000
#define BSD_MDMBUF          0x00100000

#define BSD_ECHOKE          0x00000001
#define BSD_ECHOE           0x00000002
#define BSD_ECHOK           0x00000004
#define BSD_ECHO            0x00000008
#define BSD_ECHONL          0x00000010
#define BSD_ECHOPRT         0x00000020
#define BSD_ECHOCTL         0x00000040
#define BSD_ISIG            0x00000080
#define BSD_ICANON          0x00000100
#define BSD_ALTWERASE       0x00000200
#define BSD_IEXTEN          0x00000400
#define BSD_EXTPROC         0x00000800
#define BSD_TOSTOP          0x00400000
#define BSD_FLUSHO          0x00800000
#define BSD_NOKERNINFO      0x02000000
#define BSD_PENDIN          0x20000000
#define BSD_NOFLSH          0x80000000

struct bsd_termios
{
	unsigned long c_iflag;
	unsigned long c_oflag;
	unsigned long c_cflag;
	unsigned long c_lflag;
	unsigned char c_cc[20];
	unsigned long c_ispeed;
	unsigned long c_ospeed;
};

#define BSD_B0      0
#define BSD_B50     50
#define BSD_B75     75
#define BSD_B110    110
#define BSD_B134    134
#define BSD_B150    150
#define BSD_B200    200
#define BSD_B300    300
#define BSD_B600    600
#define BSD_B1200   1200
#define BSD_B1800   1800
#define BSD_B2400   2400
#define BSD_B4800   4800
#define BSD_B9600   9600
#define BSD_B19200  19200
#define BSD_B38400  38400
#define BSD_B7200   7200
#define BSD_B14400  14400
#define BSD_B28800  28800
#define BSD_B57600  57600
#define BSD_B76800  76800
#define BSD_B115200 115200
#define BSD_B230400 230400

#define BSD_TCIFLUSH  1
#define BSD_TCOFLUSH  2
#define BSD_TCIOFLUSH 3

#endif // DARLING_EMULATION_CONVERSION_BSD_SYS_TERMIOS_H
