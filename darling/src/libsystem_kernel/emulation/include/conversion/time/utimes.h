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

#ifndef DARLING_CONVERSION_TIME_UTIMES_H
#define DARLING_CONVERSION_TIME_UTIMES_H

#include <darling/emulation/conversion/time/gettimeofday.h>

// The timespec layout utimensat() takes on the host.
struct linux_utimensat_timespec
{
	long tv_sec;
	long tv_nsec;
};

// Converts the two timevals that utimes() and futimes() take into the
// timespecs utimensat() takes.
//
// QUIRK: macOS accepts a tv_usec outside 0..999999 and carries the excess
// into the seconds: {2000, 2500000} sets 2002.5 s and {2000, -1} sets
// 1999.999999 s, with no error. Linux utimes() and utimensat() reject such
// values with EINVAL, so normalize them here.
// Evidence: black-box observation of utimes() and futimes() on macOS 27.0.1,
// arm64 and x86_64 under Rosetta (tier 2).
// Test: utimes-usec-normalize (darling tests).
static inline void utimes_timeval_to_timespec(const struct bsd_timeval tv[2],
	struct linux_utimensat_timespec ts[2])
{
	for (int i = 0; i < 2; i++) {
		long sec = (long) tv[i].tv_sec;
		long usec = tv[i].tv_usec;

		sec += usec / 1000000;
		usec %= 1000000;
		if (usec < 0) {
			usec += 1000000;
			sec--;
		}

		ts[i].tv_sec = sec;
		ts[i].tv_nsec = usec * 1000;
	}
}

#endif // DARLING_CONVERSION_TIME_UTIMES_H
