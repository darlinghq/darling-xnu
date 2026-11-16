#ifndef LINUX_MOUNT_TABLE_H
#define LINUX_MOUNT_TABLE_H

#include <stddef.h>

/**
 * Finds the mount that contains `path` (a host path) in /proc/self/mounts and
 * copies its mount point, filesystem type and source into the buffers.
 * The buffers are set to empty strings if no mount matches.
 *
 * @retval `0` on success
 * @retval a negative BSD errno if the mount table can't be read
 */
long mount_table_lookup(const char* path, char* mnton, size_t mnton_size,
	char* fstype, size_t fstype_size, char* mntfrom, size_t mntfrom_size);

/**
 * Called by mount_table_enumerate() for each mount, with its mount options (the comma-separated fourth field of
 * /proc/self/mounts, "" if there is none). The strings are only valid during the call, and the call is
 * made with the table locked: the callback must not use the mount table.
 * A nonzero return stops the enumeration and is returned by it.
 */
typedef long (*mount_table_visitor)(void* context, const char* mnton, const char* fstype, const char* mntfrom,
	const char* options);

/**
 * Calls `visit` for every mount of /proc/self/mounts in table order, except one that a later mount on the same
 * mount point covers (what a lookup of a path on that mount point finds).
 *
 * @retval `0` on success, or the nonzero value `visit` returned
 * @retval a negative BSD errno if the mount table can't be read
 */
long mount_table_enumerate(mount_table_visitor visit, void* context);

/**
 * Finds what is mounted exactly on `host_mount_point` (a host path without a trailing slash; the topmost mount if
 * there are several) and copies its filesystem type and its mount options (the comma-separated fourth field of
 * /proc/self/mounts) into the buffers.
 *
 * @retval `0` on success
 * @retval `-EINVAL` if nothing is mounted on that path
 * @retval another negative BSD errno if the mount table can't be read
 */
long mount_table_mount_at(const char* host_mount_point, char* fstype, size_t fstype_size,
	char* options, size_t options_size);

#endif // LINUX_MOUNT_TABLE_H
