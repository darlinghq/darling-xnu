// Copyright (C) 2026 Lubos Dolezel
#ifndef DARLING_MOUNT_OPTIONS_H
#define DARLING_MOUNT_OPTIONS_H

/**
 * Finds the `user_id=<n>` option, which Linux shows for a FUSE mount and which names the user who mounted it, in
 * a comma-separated option list as /proc/self/mounts gives it. The option must be a whole entry and the number a
 * plain decimal that fits in an unsigned int.
 *
 * @retval `1` if found, with the number stored in `*uid`
 * @retval `0` if not (`*uid` is left alone)
 */
int mount_options_fuse_user_id(const char* options, unsigned int* uid);

#endif // DARLING_MOUNT_OPTIONS_H
