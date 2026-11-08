/**
 * This file is part of Darling.
 *
 * Copyright (C) 2026 Darling Developers
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

#ifndef _DARLING_EMULATION_DC_IMAGE_SNAPSHOT_H_
#define _DARLING_EMULATION_DC_IMAGE_SNAPSHOT_H_

/**
 * Darling-private fcntl() command (not part of the macOS API).
 *
 * `fcntl(fd, DC_F_SNAPSHOT_IMAGE)` copies the whole regular file `fd` refers
 * to into a new anonymous file and returns a new, close-on-exec descriptor
 * for the copy. The copy is sealed: it can no longer be written, shrunk or
 * grown, and its seals cannot be changed. Mapping the copy instead of the
 * file guarantees that the bytes a signature was checked against are the
 * bytes that get mapped, even if the file changes afterwards.
 *
 * The argument is ignored. The source descriptor's file offset does not
 * change. Fails with EINVAL for an empty file, EFBIG for a file larger than
 * `DC_SNAPSHOT_IMAGE_MAX`, and with the errors of fstat(), pread() and
 * memfd creation otherwise.
 */
#define DC_F_SNAPSHOT_IMAGE 0x40004443

// the largest file DC_F_SNAPSHOT_IMAGE copies (2 GiB)
#define DC_SNAPSHOT_IMAGE_MAX (2ull * 1024 * 1024 * 1024)

#endif // _DARLING_EMULATION_DC_IMAGE_SNAPSHOT_H_
