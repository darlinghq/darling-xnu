#ifndef DARLING_EMULATION_CONVERSION_LINUX_BITS_DIRENT_H
#define DARLING_EMULATION_CONVERSION_LINUX_BITS_DIRENT_H

#ifndef DARLING_EMULATION_CONVERSION_LINUX_DIRENT_H
#error "Use <emulation/conversion/variables/linux/dirent.h> instead"
#endif

struct linux_dirent64
{
	unsigned long long d_ino;
	unsigned long long d_off;
	unsigned short d_reclen;
	unsigned char d_type;
	char d_name[];
};

#endif // DARLING_EMULATION_CONVERSION_LINUX_BITS_DIRENT_H
