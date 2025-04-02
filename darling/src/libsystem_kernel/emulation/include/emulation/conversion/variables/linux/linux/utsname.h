#ifndef DARLING_EMULATION_CONVERSION_LINUX_LINUX_UTSNAME_H
#define DARLING_EMULATION_CONVERSION_LINUX_LINUX_UTSNAME_H

#define LINUX___NEW_UTS_LEN 64

struct linux_new_utsname {
	char sysname[LINUX___NEW_UTS_LEN + 1];
	char nodename[LINUX___NEW_UTS_LEN + 1];
	char release[LINUX___NEW_UTS_LEN + 1];
	char version[LINUX___NEW_UTS_LEN + 1];
	char machine[LINUX___NEW_UTS_LEN + 1];
	char domainname[LINUX___NEW_UTS_LEN + 1];
};

#endif // DARLING_EMULATION_CONVERSION_LINUX_LINUX_TYPES_H
