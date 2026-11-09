/**
 * This file is part of Darling.
 *
 * Copyright (C) 2026 Lubos Dolezel
 *
 * Darling is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Darling is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Darling.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DARLING_EMULATION_BPF_DEVICE_H
#define DARLING_EMULATION_BPF_DEVICE_H

// /dev/bpfN packet capture devices (bpf(4)), emulated on Linux AF_PACKET
// sockets. Each open device is a packet socket; the BPF state that XNU keeps
// in the kernel is kept per open file in this process.

// Number of descriptors that currently refer to BPF devices; the hooks below
// return at once while it is zero.
extern volatile int bpf_device_fd_count;

// If expanded_path (a host path from vchroot_expand) names the guest's
// /dev/bpfN, returns N, otherwise -1.
int bpf_device_minor(const char* expanded_path);

// Opens BPF device `minor` with Darwin open(2) flags. Returns the new
// descriptor or a negative Darwin errno.
long bpf_device_open(int minor, int bsd_flags);

// Descriptor bookkeeping: close (called before the descriptor is closed),
// dup/dup2/F_DUPFD (called after newfd was created), and the child side of
// fork.
void bpf_device_close(int fd);
void bpf_device_dup(int oldfd, int newfd);
void bpf_device_postfork_child(void);

// read(2), write(2) and ioctl(2) on a BPF descriptor. Each returns 0 and
// leaves *retval untouched if fd is not a BPF device.
int bpf_device_read(int fd, void* buf, int len, long* retval);
int bpf_device_write(int fd, const void* buf, int len, long* retval);
int bpf_device_ioctl(int fd, unsigned int cmd, void* arg, int* retval);

// For fstat(2): if fd is a BPF device, sets *mode and *rdev to describe a
// character device and returns 1.
int bpf_device_stat(int fd, unsigned int* mode, int* rdev);

#endif // DARLING_EMULATION_BPF_DEVICE_H
