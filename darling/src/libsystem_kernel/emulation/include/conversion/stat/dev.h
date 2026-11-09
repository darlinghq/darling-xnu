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

#ifndef DARLING_CONVERSION_STAT_DEV_H
#define DARLING_CONVERSION_STAT_DEV_H

// Device numbers use different encodings on the two sides.
//
// Darwin (sys/types.h major()/minor()/makedev()): a 32-bit dev_t with the
// major number in bits 31..24 and the minor number in bits 23..0.
//
// Linux (the kernel's new_encode_dev() and glibc's sysmacros.h): the low 8
// bits of the minor in bits 7..0, the major in bits 19..8, the rest of the
// minor in bits 31..20, and (64-bit dev_t only) the rest of the major from
// bit 44 up.

static inline unsigned int dev_linux_to_bsd(unsigned long long ldev)
{
	unsigned int maj = ((ldev >> 8) & 0xfff) | ((unsigned int)(ldev >> 32) & ~0xfffu);
	unsigned int min = (ldev & 0xff) | ((unsigned int)(ldev >> 12) & 0xfff00);

	// A Darwin dev_t has room for 8 bits of major and 24 bits of minor.
	return ((maj & 0xff) << 24) | (min & 0xffffff);
}

static inline unsigned int dev_bsd_to_linux(unsigned int bdev)
{
	unsigned int maj = (bdev >> 24) & 0xff;
	unsigned int min = bdev & 0xffffff;

	// The kernel's mknod takes a 32-bit dev, which holds 20 bits of minor.
	return (min & 0xff) | (maj << 8) | ((min & 0xfff00) << 12);
}

#endif // DARLING_CONVERSION_STAT_DEV_H
