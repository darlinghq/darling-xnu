// Copyright (C) 2026 Lubos Dolezel
#ifndef DARLING_GUEST_MOUNT_H
#define DARLING_GUEST_MOUNT_H

#include <darling/emulation/conversion/stat/common.h>

//
// A guest sees a tree of paths, but /proc/self/mounts lists the host's mount points. These functions name the
// mounts the way a guest can use the names:
//  - the mount that holds the guest root is the root volume and is "/", as on macOS;
//  - a mount below the guest root keeps its guest path;
//  - any other host mount is outside the guest tree: it is named below /Volumes/SystemRoot, where the host's
//    whole tree is reachable from the guest, and it is not listed by getfsstat.
//

/**
 * Sets f_mntonname, f_fstypename and f_mntfromname of `buf` for the mount that holds `host_path` (a host path).
 *
 * @retval `0` on success
 * @retval a negative BSD errno if the mount table or the guest root can't be read
 */
long guest_mount_describe(const char* host_path, struct bsd_statfs64* buf);

/**
 * Writes the name of the volume that holds `host_path` (a host path) to `name`, NUL-terminated, for
 * getattrlist(ATTR_VOL_NAME): the name a FUSE file system gives in the "darling.volume.name" xattr of its root (as
 * darling-dmg does for disk images); "Macintosh HD" for the root volume, the default name of a Mac's startup volume;
 * otherwise the last component of the volume's guest path.
 *
 * @retval `0` on success
 * @retval a negative BSD errno if the mount table or the guest root can't be read
 */
long guest_mount_volume_name(const char* host_path, char* name, __SIZE_TYPE__ name_size);

/**
 * Fills `buf` with the statfs of each volume visible to the guest, the root volume first, and returns how many
 * were written (at most bufsize / sizeof(struct bsd_statfs64)). With a NULL buffer returns how many there are.
 *
 * @retval the count, or a negative BSD errno if the mount table or the guest root can't be read
 */
long guest_mount_list(struct bsd_statfs64* buf, int bufsize);

/**
 * Like guest_mount_list(), for the entries of the legacy struct statfs (the one 32-bit processes get from
 * getfsstat without 64-bit inodes); bufsize is in bytes of that structure.
 */
long guest_mount_list_legacy(struct bsd_statfs* buf, int bufsize);

#endif // DARLING_GUEST_MOUNT_H
