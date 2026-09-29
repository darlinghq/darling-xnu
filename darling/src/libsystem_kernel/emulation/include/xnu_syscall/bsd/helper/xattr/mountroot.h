#ifndef DARLING_ATTRIBUTE_MOUNTROOT_H
#define DARLING_ATTRIBUTE_MOUNTROOT_H
#include "mountpoint.h"

// Linux statx has a fixed 256-byte ABI, with mount ID at byte 144.
struct attribute_mount_statx {
	uint32_t mask;
	unsigned char unused[140];
	uint64_t mount_id;
	unsigned char remainder[104];
};
_Static_assert(sizeof(struct attribute_mount_statx) == 256, "Linux statx size");
_Static_assert(offsetof(struct attribute_mount_statx, mount_id) == 144, "Linux mount ID offset");

static int attribute_fd_mount_id(int fd, uint64_t* mount_id)
{
#ifdef __NR_statx
	struct attribute_mount_statx info = {0};
	int result = LINUX_SYSCALL(__NR_statx, fd, "", 0x1000, 0x1000, &info);
	// AT_EMPTY_PATH and STATX_MNT_ID both have value 0x1000.
	if (result < 0)
		return errno_linux_to_bsd(result);
	if (!(info.mask & 0x1000))
		return -ENOTSUP;
	*mount_id = info.mount_id;
	return 0;
#else
	return -ENOTSUP;

#endif
}

static long attribute_read_mountinfo(void* context, char* bytes, __SIZE_TYPE__ capacity)
{
	long result = LINUX_SYSCALL(__NR_read, *(int*)context, bytes, capacity);
	return result < 0 ? errno_linux_to_bsd(result) : result;
}

// Return an owned CLOEXEC O_PATH descriptor, or a negative BSD errno.
// The input descriptor remains borrowed. Caller supplies bounded record storage.
// This finds the host mount root; guest namespace clamping belongs to the caller.
static int attribute_open_mount_root(int fd, char* record, __SIZE_TYPE__ record_capacity)
{
	uint64_t wanted, actual;
	int result = attribute_fd_mount_id(fd, &wanted);
	if (result < 0)
		return result;
	int mounts = LINUX_SYSCALL(__NR_openat, -100, "/proc/self/mountinfo",
		LINUX_O_RDONLY | LINUX_O_CLOEXEC, 0);
	if (mounts < 0)
		return errno_linux_to_bsd(mounts);
	char path[4096];
	result = attribute_find_mountpoint(attribute_read_mountinfo, &mounts,
		wanted, path, sizeof(path), record, record_capacity);
	LINUX_SYSCALL(__NR_close, mounts);
	if (result < 0)
		return result;
	int root = LINUX_SYSCALL(__NR_openat, -100, path, 010000000 | LINUX_O_CLOEXEC, 0);
	if (root < 0)
		return errno_linux_to_bsd(root);
	// A mount can be replaced between reading mountinfo and opening its path.
	result = attribute_fd_mount_id(root, &actual);
	if (result < 0 || actual != wanted) {
		LINUX_SYSCALL(__NR_close, root);
		return result < 0 ? result : -ENOENT;
	}
	return root;
}
// Obtain the kernel namespace spelling, not the /Volumes/SystemRoot guest alias.
static int attribute_host_fdpath(int fd, char* output, __SIZE_TYPE__ capacity)
{
	if (capacity < 2)
		return -ENAMETOOLONG;
	char proc[64];
	__simple_sprintf(proc, "/proc/self/fd/%d", fd);
	long result = LINUX_SYSCALL(__NR_readlinkat, -100, proc, output, capacity - 1);
	if (result < 0)
		return errno_linux_to_bsd(result);
	if ((__SIZE_TYPE__)result >= capacity - 1)
		return -ENAMETOOLONG;
	output[result] = '\0';
	return 0;
}

// A virtual guest root inside a host filesystem acts as that guest's volume
// root. Preserve actual mount roots for nested mounts and host escape paths.
static int attribute_open_volume_root(int fd, int guest_root,
	char* record, __SIZE_TYPE__ record_capacity)
{
	uint64_t object_mount, guest_mount;
	int result = attribute_fd_mount_id(fd, &object_mount);
	if (result < 0)
		return result;
	result = attribute_fd_mount_id(guest_root, &guest_mount);
	if (result < 0)
		return result;
	if (object_mount == guest_mount) {
		char object_path[4096], guest_path[4096];
		result = attribute_host_fdpath(fd, object_path, sizeof(object_path));
		if (result < 0)
			return result;
		result = attribute_host_fdpath(guest_root, guest_path, sizeof(guest_path));
		if (result < 0)
			return result;
		__SIZE_TYPE__ length = strlen(guest_path);
		int inside = (length == 1 && guest_path[0] == '/') ||
			(strncmp(object_path, guest_path, length) == 0 &&
			 (object_path[length] == '\0' || object_path[length] == '/'));
		if (inside) {
			result = LINUX_SYSCALL(__NR_openat, guest_root, ".",
				010000000 | LINUX_O_CLOEXEC, 0);
			return result < 0 ? errno_linux_to_bsd(result) : result;
		}
	}
	return attribute_open_mount_root(fd, record, record_capacity);
}
#endif
