/*
This file is part of Darling.

Copyright (C) 2026 Lubos Dolezel

Darling is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Darling is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Darling.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _DARLING_TSD_TRAP_H_
#define _DARLING_TSD_TRAP_H_

#include <stdbool.h>
#include <stdint.h>

// Darling-private ARM64 encoding: dyld replaces a Darwin `mrs Xd, TPIDRRO_EL0`
// that it cannot rewrite in place by `udf #(0xda00 + d)`. UDF (Arm
// Architecture Reference Manual, DDI 0487) is permanently undefined, so the
// encoding space cannot hold real code. The SIGILL handler recognizes the word,
// writes the Darwin TSD base into Xd, and skips the instruction.
#define DARLING_TSD_TRAP_BASE 0x0000DA00u
#define DARLING_TSD_TRAP_MASK 0xFFFFFFE0u

// Builds the trapping instruction for destination register `reg` (0-31).
static inline uint32_t darling_tsd_trap_encode(uint32_t reg)
{
	return DARLING_TSD_TRAP_BASE | (reg & 0x1F);
}

// True if `insn` is a TSD trap; stores the destination register (31 is XZR,
// whose write is discarded) in `*reg`.
static inline bool darling_tsd_trap_decode(uint32_t insn, uint32_t* reg)
{
	if ((insn & DARLING_TSD_TRAP_MASK) != DARLING_TSD_TRAP_BASE)
		return false;
	*reg = insn & 0x1F;
	return true;
}

#endif // _DARLING_TSD_TRAP_H_
