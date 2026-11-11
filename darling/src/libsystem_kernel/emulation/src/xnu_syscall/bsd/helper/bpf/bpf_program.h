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

#ifndef DARLING_EMULATION_BPF_PROGRAM_H
#define DARLING_EMULATION_BPF_PROGRAM_H

// Classic BPF filter programs, as installed with BIOCSETF on a /dev/bpf
// device. This header and bpf_program.c depend on nothing but <stdint.h>, so
// that the interpreter can also be built and tested on the host.

#include <stdint.h>

// One instruction; the same layout as Darwin's struct bpf_insn and Linux's
// struct sock_filter.
struct bpf_program_insn {
	uint16_t code;
	uint8_t jt;
	uint8_t jf;
	uint32_t k;
};

// Most instructions a program may have (BPF_MAXINSNS in <net/bpf.h>).
#define BPF_PROGRAM_MAX_INSNS 512
// Scratch memory words available to a program (BPF_MEMWORDS).
#define BPF_PROGRAM_MEMWORDS 16

// Checks a program before it is installed: 1 to 512 instructions, packet
// offsets below max_offset, scratch memory indices in range, no division by
// a constant zero, only forward jumps that stay inside the program, and a
// return instruction at the end. Returns 1 if the program is acceptable.
int bpf_program_check(const struct bpf_program_insn* prog, uint32_t count, uint32_t max_offset);

// Runs a program that passed bpf_program_check over a packet. wirelen is the
// packet's original length, caplen the number of bytes present at pkt.
// Returns the number of bytes to capture; 0 rejects the packet.
uint32_t bpf_program_run(const struct bpf_program_insn* prog, uint32_t count,
	const uint8_t* pkt, uint32_t wirelen, uint32_t caplen);

#endif // DARLING_EMULATION_BPF_PROGRAM_H
