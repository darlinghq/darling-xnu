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

#endif // LINUX_MOUNT_TABLE_H
