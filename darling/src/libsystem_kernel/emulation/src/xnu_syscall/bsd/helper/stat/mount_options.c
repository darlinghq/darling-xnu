// Copyright (C) 2026 Lubos Dolezel
// Self-contained (no libc), so that it can also be built into a host-side unit test.
#include <darling/emulation/xnu_syscall/bsd/helper/stat/mount_options.h>

static const char user_id_option[] = "user_id=";

int mount_options_fuse_user_id(const char* options, unsigned int* uid)
{
	const char* entry = options;

	while (*entry)
	{
		const char* end = entry;
		while (*end && *end != ',')
			end++;

		unsigned int i = 0;
		while (user_id_option[i] && entry + i < end && entry[i] == user_id_option[i])
			i++;

		if (user_id_option[i] == 0 && entry + i < end)
		{
			unsigned long long value = 0;
			const char* p = entry + i;

			while (p < end && *p >= '0' && *p <= '9' && value <= 0xffffffffull)
				value = value * 10 + (unsigned) (*p++ - '0');

			if (p == end && value <= 0xffffffffull)
			{
				*uid = (unsigned int) value;
				return 1;
			}
		}

		entry = *end ? end + 1 : end;
	}
	return 0;
}
