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

// A classic BPF interpreter, written from the instruction set described in
// bpf(4) and <net/bpf.h>. A program sees an accumulator A, an index register
// X, 16 words of scratch memory and the packet; every word is unsigned and
// multi-byte packet loads are big-endian.

#include "bpf_program.h"

// Instruction fields.
#define OP_CLASS(c) ((c) & 0x07)
#define OP_SIZE(c)  ((c) & 0x18)
#define OP_MODE(c)  ((c) & 0xe0)
#define OP_ALU(c)   ((c) & 0xf0)
#define OP_SRC_X(c) (((c) & 0x08) != 0)
#define OP_RVAL(c)  ((c) & 0x18)
#define OP_MISC(c)  ((c) & 0xf8)

enum {
	CLASS_LD = 0, CLASS_LDX = 1, CLASS_ST = 2, CLASS_STX = 3,
	CLASS_ALU = 4, CLASS_JMP = 5, CLASS_RET = 6, CLASS_MISC = 7,
};

enum { SIZE_W = 0x00, SIZE_H = 0x08, SIZE_B = 0x10 };

enum {
	MODE_IMM = 0x00, MODE_ABS = 0x20, MODE_IND = 0x40,
	MODE_MEM = 0x60, MODE_LEN = 0x80, MODE_MSH = 0xa0,
};

enum {
	ALU_ADD = 0x00, ALU_SUB = 0x10, ALU_MUL = 0x20, ALU_DIV = 0x30,
	ALU_OR = 0x40, ALU_AND = 0x50, ALU_LSH = 0x60, ALU_RSH = 0x70,
	ALU_NEG = 0x80,
};

enum { JMP_JA = 0x00, JMP_JEQ = 0x10, JMP_JGT = 0x20, JMP_JGE = 0x30, JMP_JSET = 0x40 };

enum { RVAL_K = 0x00, RVAL_A = 0x10 };

enum { MISC_TAX = 0x00, MISC_TXA = 0x80 };

int bpf_program_check(const struct bpf_program_insn* prog, uint32_t count, uint32_t max_offset)
{
	if (prog == 0 || count == 0 || count > BPF_PROGRAM_MAX_INSNS)
		return 0;

	for (uint32_t pc = 0; pc < count; pc++)
	{
		const struct bpf_program_insn* in = &prog[pc];
		// Jump targets are relative to the following instruction and must
		// land inside the program; count is small, so this cannot wrap.
		uint32_t remaining = count - (pc + 1);

		switch (OP_CLASS(in->code))
		{
			case CLASS_LD:
			case CLASS_LDX:
				switch (OP_MODE(in->code))
				{
					case MODE_IMM:
					case MODE_LEN:
						break;
					case MODE_ABS:
					case MODE_IND:
					case MODE_MSH:
						if (in->k >= max_offset)
							return 0;
						break;
					case MODE_MEM:
						if (in->k >= BPF_PROGRAM_MEMWORDS)
							return 0;
						break;
					default:
						return 0;
				}
				break;

			case CLASS_ST:
			case CLASS_STX:
				if (in->k >= BPF_PROGRAM_MEMWORDS)
					return 0;
				break;

			case CLASS_ALU:
				switch (OP_ALU(in->code))
				{
					case ALU_DIV:
						if (!OP_SRC_X(in->code) && in->k == 0)
							return 0;
						break;
					case ALU_ADD: case ALU_SUB: case ALU_MUL: case ALU_OR:
					case ALU_AND: case ALU_LSH: case ALU_RSH: case ALU_NEG:
						break;
					default:
						return 0;
				}
				break;

			case CLASS_JMP:
				switch (OP_ALU(in->code))
				{
					case JMP_JA:
						if (in->k >= remaining)
							return 0;
						break;
					case JMP_JEQ: case JMP_JGT: case JMP_JGE: case JMP_JSET:
						if (in->jt >= remaining || in->jf >= remaining)
							return 0;
						break;
					default:
						return 0;
				}
				break;

			case CLASS_RET:
			case CLASS_MISC:
				break;
		}
	}

	return OP_CLASS(prog[count - 1].code) == CLASS_RET;
}

// Reads a big-endian value of `width` bytes at `offset`; fails when any of
// those bytes lies outside the captured data.
static int load_packet(const uint8_t* pkt, uint32_t caplen, uint32_t offset, uint32_t width, uint32_t* value)
{
	if (offset > caplen || width > caplen - offset)
		return 0;

	uint32_t v = 0;
	for (uint32_t i = 0; i < width; i++)
		v = (v << 8) | pkt[offset + i];

	*value = v;
	return 1;
}

static uint32_t size_width(uint16_t code)
{
	switch (OP_SIZE(code))
	{
		case SIZE_W: return 4;
		case SIZE_H: return 2;
		case SIZE_B: return 1;
		default:     return 0;
	}
}

uint32_t bpf_program_run(const struct bpf_program_insn* prog, uint32_t count,
	const uint8_t* pkt, uint32_t wirelen, uint32_t caplen)
{
	uint32_t a = 0, x = 0;
	uint32_t mem[BPF_PROGRAM_MEMWORDS] = { 0 };
	uint32_t pc = 0;

	// A checked program only jumps forward and ends with a return, so this
	// loop always terminates; the bound is a second line of defence.
	while (pc < count)
	{
		const struct bpf_program_insn* in = &prog[pc++];
		uint32_t k = in->k;
		uint32_t value;

		switch (OP_CLASS(in->code))
		{
			case CLASS_LD:
				switch (OP_MODE(in->code))
				{
					case MODE_ABS:
					case MODE_IND:
					{
						uint32_t width = size_width(in->code);
						uint32_t offset = k;
						if (width == 0)
							return 0;
						if (OP_MODE(in->code) == MODE_IND)
						{
							offset = x + k;
							// An index that wraps around is out of the packet.
							if (offset < x)
								return 0;
						}
						if (!load_packet(pkt, caplen, offset, width, &value))
							return 0;
						a = value;
						break;
					}
					case MODE_IMM:
						if (OP_SIZE(in->code) != SIZE_W)
							return 0;
						a = k;
						break;
					case MODE_MEM:
						if (OP_SIZE(in->code) != SIZE_W)
							return 0;
						a = mem[k];
						break;
					case MODE_LEN:
						if (OP_SIZE(in->code) != SIZE_W)
							return 0;
						a = wirelen;
						break;
					default:
						return 0;
				}
				break;

			case CLASS_LDX:
				switch (OP_MODE(in->code))
				{
					case MODE_IMM:
						if (OP_SIZE(in->code) != SIZE_W)
							return 0;
						x = k;
						break;
					case MODE_MEM:
						if (OP_SIZE(in->code) != SIZE_W)
							return 0;
						x = mem[k];
						break;
					case MODE_LEN:
						if (OP_SIZE(in->code) != SIZE_W)
							return 0;
						x = wirelen;
						break;
					case MODE_MSH:
						// Four times the low nibble of a byte: the length of
						// an IPv4 header.
						if (OP_SIZE(in->code) != SIZE_B)
							return 0;
						if (!load_packet(pkt, caplen, k, 1, &value))
							return 0;
						x = (value & 0x0f) << 2;
						break;
					default:
						return 0;
				}
				break;

			case CLASS_ST:
				mem[k] = a;
				break;

			case CLASS_STX:
				mem[k] = x;
				break;

			case CLASS_ALU:
			{
				uint32_t operand = OP_SRC_X(in->code) ? x : k;
				switch (OP_ALU(in->code))
				{
					case ALU_ADD: a += operand; break;
					case ALU_SUB: a -= operand; break;
					case ALU_MUL: a *= operand; break;
					case ALU_DIV:
						// Dividing by a zero X rejects the packet.
						if (operand == 0)
							return 0;
						a /= operand;
						break;
					case ALU_OR:  a |= operand; break;
					case ALU_AND: a &= operand; break;
					// Shift counts are taken modulo 32, as the 32-bit shift
					// instructions of the CPUs Darwin runs on do.
					case ALU_LSH: a <<= (operand & 31); break;
					case ALU_RSH: a >>= (operand & 31); break;
					case ALU_NEG: a = 0u - a; break;
					default:
						return 0;
				}
				break;
			}

			case CLASS_JMP:
			{
				uint32_t operand = OP_SRC_X(in->code) ? x : k;
				int taken;
				switch (OP_ALU(in->code))
				{
					case JMP_JA:
						pc += k;
						continue;
					case JMP_JEQ:  taken = a == operand; break;
					case JMP_JGT:  taken = a > operand; break;
					case JMP_JGE:  taken = a >= operand; break;
					case JMP_JSET: taken = (a & operand) != 0; break;
					default:
						return 0;
				}
				pc += taken ? in->jt : in->jf;
				break;
			}

			case CLASS_RET:
				switch (OP_RVAL(in->code))
				{
					case RVAL_K: return k;
					case RVAL_A: return a;
					default:     return 0;
				}

			case CLASS_MISC:
				switch (OP_MISC(in->code))
				{
					case MISC_TAX: x = a; break;
					case MISC_TXA: a = x; break;
					default:
						return 0;
				}
				break;
		}
	}

	return 0;
}
