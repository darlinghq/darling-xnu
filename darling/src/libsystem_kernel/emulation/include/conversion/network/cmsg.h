/*
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

#ifndef DARLING_CONVERSION_NETWORK_CMSG_H
#define DARLING_CONVERSION_NETWORK_CMSG_H

#include <stddef.h>

/*
 * Ancillary data (sendmsg/recvmsg control messages) between the Darwin and
 * the Linux layout.
 *
 * Darwin's struct cmsghdr has a 4-byte cmsg_len and aligns every header and
 * payload to 4 bytes on all architectures. Linux uses a size_t cmsg_len and
 * aligns to sizeof(long): the layouts agree on i386 but not on x86_64 or
 * arm64. The level and type numbers differ on every architecture
 * (SOL_SOCKET is 0xffff on Darwin and 1 on Linux, SCM_TIMESTAMP is 2 and 29,
 * the IP and IPv6 types are numbered differently), so every message is
 * rewritten, never passed through.
 */

/*
 * Checks the Darwin control buffer and returns the size of its Linux
 * equivalent (0 if nothing is left to send after dropping types that have no
 * Linux counterpart), or a negative errno:
 *   -EINVAL if the buffer is shorter than a header, if a cmsg_len is shorter
 *           than a header or runs past the buffer, if bytes too short for a
 *           header trail the last message, if an SCM_RIGHTS payload is not a
 *           whole number of descriptors, or for a SOL_SOCKET type other than
 *           SCM_RIGHTS.
 */
long cmsg_bsd_to_linux_size(const void* bsd_control, size_t bsd_len);

/*
 * Writes the Linux equivalent of a buffer accepted by cmsg_bsd_to_linux_size()
 * into linux_control, which must be at least as large as that function
 * returned. Returns the number of bytes written.
 */
size_t cmsg_bsd_to_linux(const void* bsd_control, size_t bsd_len, void* linux_control);

/*
 * Size of the Linux buffer to receive into for a Darwin buffer of bsd_len
 * bytes: large enough that everything which fits the Darwin buffer once
 * converted also fits the Linux one. 0 if bsd_len cannot hold even a header.
 */
size_t cmsg_linux_space_for_bsd(size_t bsd_len);

/*
 * Converts received Linux control messages into the caller's Darwin buffer.
 * Messages that do not fit are left out and *truncated is set to 1; any
 * descriptors carried in SCM_RIGHTS messages (or the part of one) that did not
 * fit are closed, so they never leak into the process without the caller
 * knowing their numbers. Types Darwin has no equivalent for are dropped.
 * Returns the number of bytes used in bsd_control (the new msg_controllen).
 */
size_t cmsg_linux_to_bsd(const void* linux_control, size_t linux_len,
		void* bsd_control, size_t bsd_len, int* truncated);

#endif // DARLING_CONVERSION_NETWORK_CMSG_H
