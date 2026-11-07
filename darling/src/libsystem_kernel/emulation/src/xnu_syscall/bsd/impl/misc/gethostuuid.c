#include <darling/emulation/xnu_syscall/bsd/impl/misc/gethostuuid.h>

#include <sys/errno.h>

#include <darling/emulation/xnu_syscall/bsd/helper/misc/platform_identity.h>

// The host UUID is the platform UUID (IOPlatformUUID), which darlingserver
// derives from the Linux machine ID or takes from the configuration.
long sys_gethostuuid(unsigned char* uuid_buf, void* timeout)
{
	const dserver_platform_identity_t* identity = platform_identity();
	int i;

	if (!identity)
		return -EIO;

	// all zero: the host has no machine ID and none was configured
	for (i = 0; i < sizeof(identity->platform_uuid); i++)
	{
		if (identity->platform_uuid[i] != 0)
			break;
	}
	if (i == sizeof(identity->platform_uuid))
		return -ENOENT;

	__builtin_memcpy(uuid_buf, identity->platform_uuid, sizeof(identity->platform_uuid));
	return 0;
}
