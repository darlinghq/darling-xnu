#ifndef EXT_FILE_HANDLE_H
#define EXT_FILE_HANDLE_H

#include <stdint.h>

#include <darling/emulation/common/base.h>

CPP_EXTERN_BEGIN

struct file_handle
{
	uint32_t handle_bytes;
	int handle_type;
	uint8_t f_handle[1];
};

typedef struct {
	union
	{
		uint8_t opaque[80];

		struct
		{
			int mount_id;

			union
			{
				struct file_handle fh;

				// Identifies the file itself, so all references to a file are identical.
				// Not 64-bit fields: that would move everything else in this union.
				struct
				{
					uint32_t device;
					uint32_t inode_low;
					uint32_t inode_high;
				};
			};
		};
	};

} RefData;

int sys_name_to_handle(const char* name, RefData* ref, int follow);
//int sys_open_by_handle(RefData* ref, int flags);
int sys_handle_to_name(RefData* ref, char name[4096]);

CPP_EXTERN_END

#endif // EXT_FILE_HANDLE_H
