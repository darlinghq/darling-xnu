#ifndef EXT_FILE_HANDLE_H
#define EXT_FILE_HANDLE_H
#include <stdint.h>

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

				struct
				{
					int index;
					int gen;
				};
			};
		};
	};

} RefData;

// Like open_by_handle_at, but provides a path and doesn't need CAP_DAC_READ_SEARCH
struct handle_to_path_args
{
	int mfd; // in
	//int mntid; // in
	char fh[80]; // in
	char path[4096]; // out
};

#ifdef __cplusplus
extern "C" {
#endif

int sys_name_to_handle(const char* name, RefData* ref, int follow);
//int sys_open_by_handle(RefData* ref, int flags);
int sys_handle_to_name(RefData* ref, char name[4096]);

#ifdef __cplusplus
}
#endif

#endif
