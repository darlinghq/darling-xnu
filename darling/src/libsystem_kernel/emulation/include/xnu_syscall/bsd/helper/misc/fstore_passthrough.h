#ifndef DARLING_FSTORE_PASSTHROUGH_H
#define DARLING_FSTORE_PASSTHROUGH_H

// Dispatches Darwin storage-control commands that have a Linux host analogue.
// Returns ENOTTY for commands that are not handled here, so callers can keep
// the Darwin "file system does not implement this command" behavior.
long darling_fstore_passthrough(int fd, unsigned long cmd, void* data);

#endif // DARLING_FSTORE_PASSTHROUGH_H
