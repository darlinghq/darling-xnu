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

#ifndef _LINK_COPY_H
#define _LINK_COPY_H

#include <darling/emulation/conversion/fcntl/open.h>
#include <darling/emulation/conversion/common_at.h>

// The rules of the copy linkat makes when the host refuses a hard link (see
// linkat.c), kept apart so they can be tested on any host.

// Flags for opening the file to be copied. The final component is followed only
// when the caller asked linkat to follow it. The file is opened without blocking
// and without becoming a controlling terminal, so that naming a FIFO or a device
// cannot hang the caller or have the side effects of opening it; only a regular
// file is copied after that.
static inline int link_copy_source_flags(int linux_flags)
{
	return LINUX_O_RDONLY | LINUX_O_CLOEXEC | LINUX_O_NONBLOCK | LINUX_O_NOCTTY
		| ((linux_flags & LINUX_AT_SYMLINK_FOLLOW) ? 0 : LINUX_O_NOFOLLOW);
}

// Permissions of the copy. It belongs to the caller, not to the owner of the
// original, so set-user-ID and set-group-ID bits (which would name the caller)
// are not carried over, as cp(1) does when it cannot keep the owner.
static inline unsigned int link_copy_mode(unsigned int st_mode)
{
	return st_mode & 0777;
}

#endif // _LINK_COPY_H
