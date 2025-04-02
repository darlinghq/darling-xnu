#ifndef DARLING_EMULATION_CONVERSION_BSD_SYS_DIRENT_H
#define DARLING_EMULATION_CONVERSION_BSD_SYS_DIRENT_H

#include <emulation/conversion/variables/bsd/sys/_types.h>

#include <emulation/conversion/variables/bsd/sys/_types/_ino_t.h>

#define BSD___DARWIN_MAXNAMLEN      255

struct bsd_dirent
{
	bsd_ino32_t d_ino;
	bsd___uint16_t d_reclen;
	bsd___uint8_t d_type;
	bsd___uint8_t d_namlen;
	char d_name[BSD___DARWIN_MAXNAMLEN + 1];
};

#define BSD___DARWIN_MAXPATHLEN     1024

struct bsd_dirent64
{
	bsd___uint64_t d_ino;
	bsd___uint64_t d_seekoff;
	bsd___uint16_t d_reclen;
	bsd___uint16_t d_namlen;
	bsd___uint8_t d_type;
	char d_name[BSD___DARWIN_MAXPATHLEN];
};

#endif // DARLING_EMULATION_CONVERSION_BSD_SYS_DIRENT_H