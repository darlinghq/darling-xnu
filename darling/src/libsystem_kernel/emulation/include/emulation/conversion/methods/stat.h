#ifndef DARLING_EMULATION_CONVERSION_STAT_H
#define DARLING_EMULATION_CONVERSION_STAT_H

#include <emulation/conversion/variables/bsd/sys/mount.h>
#include <emulation/conversion/variables/bsd/sys/stat.h>
#include <emulation/conversion/variables/linux/asm/stat.h>
#include <emulation/conversion/variables/linux/asm/statfs.h>

void stat_linux_to_bsd(const struct linux_stat* lstat, struct bsd_stat* stat);
void stat_linux_to_bsd64(const struct linux_stat* lstat, struct bsd_stat64* stat);
void statfs_linux_to_bsd(const struct linux_statfs64* lstat, struct bsd_statfs* stat);
void statfs_linux_to_bsd64(const struct linux_statfs64* lstat, struct bsd_statfs64* stat);

#endif // DARLING_EMULATION_CONVERSION_STAT_H
