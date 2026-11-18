#ifndef _SYSCTL_PROC_H
#define _SYSCTL_PROC_H

#include <stdbool.h>
#include <stdint.h>

// Utility functions for parsing Linux procfs /proc/XX/stat
bool read_string(const char* path, char* dst, int maxlen);
const char* next_stat_elem(char** buf);
void skip_stat_elems(char** buf, int count);
bool proc_start_time(uint64_t starttime, uint64_t* sec, uint64_t* usec);

// The macOS process name: the basename of the process's executable, truncated to
// fit `size` (a NUL is always written). Returns false when no name is known
// (e.g. host processes); callers then keep the Linux comm.
bool darling_process_name(int pid, char* out, unsigned long size);

#endif // _SYSCTL_PROC_H
