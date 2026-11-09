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

// Host test for the classic BPF checker and interpreter behind /dev/bpf.
// The programs are written here from the instruction encodings in bpf(4)
// and <net/bpf.h>.

#include "../src/libsystem_kernel/emulation/src/xnu_syscall/bsd/helper/bpf/bpf_program.c"

#include <stdio.h>
#include <string.h>

#define S(code, k) { (uint16_t)(code), 0, 0, (uint32_t)(k) }
#define J(code, k, jt, jf) { (uint16_t)(code), (jt), (jf), (uint32_t)(k) }
#define N(a) ((uint32_t)(sizeof(a) / sizeof((a)[0])))

// Encodings
#define LD_W_ABS 0x20
#define LD_H_ABS 0x28
#define LD_B_ABS 0x30
#define LD_W_IND 0x40
#define LD_H_IND 0x48
#define LD_W_LEN 0x80
#define LD_IMM   0x00
#define LD_MEM   0x60
#define LDX_IMM  0x01
#define LDX_B_MSH 0xb1
#define ST       0x02
#define STX      0x03
#define ALU_ADD_K 0x04
#define ALU_SUB_X 0x1c
#define ALU_MUL_K 0x24
#define ALU_DIV_K 0x34
#define ALU_DIV_X 0x3c
#define ALU_AND_K 0x54
#define ALU_LSH_K 0x64
#define ALU_RSH_X 0x7c
#define ALU_NEG   0x84
#define ALU_MOD_K 0x94
#define JA        0x05
#define JEQ_K     0x15
#define JGT_K     0x25
#define JSET_K    0x45
#define JEQ_X     0x1d
#define RET_K     0x06
#define RET_A     0x16
#define TAX       0x07
#define TXA       0x87

static int failures;

#define CHECK(cond) do { if (!(cond)) { fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #cond); failures++; } } while (0)

// "udp and dst port <port>" over an IPv4 packet that starts at `nh`
// (14 for Ethernet, 4 for DLT_NULL), with the IP header length taken from
// the packet. Returns the program length.
static uint32_t udp_port_filter(struct bpf_program_insn* p, uint32_t nh, uint32_t port, int ether)
{
	uint32_t n = 0;
	if (ether)
	{
		p[n++] = (struct bpf_program_insn) S(LD_H_ABS, 12);
		p[n++] = (struct bpf_program_insn) J(JEQ_K, 0x0800, 0, 6);
	}
	else
	{
		p[n++] = (struct bpf_program_insn) S(LD_W_ABS, 0);
		// DLT_NULL family is host order; this test runs little-endian.
		p[n++] = (struct bpf_program_insn) J(JEQ_K, 0x02000000, 0, 6);
	}
	p[n++] = (struct bpf_program_insn) S(LD_B_ABS, nh + 9);
	p[n++] = (struct bpf_program_insn) J(JEQ_K, 17, 0, 4);
	p[n++] = (struct bpf_program_insn) S(LDX_B_MSH, nh);
	p[n++] = (struct bpf_program_insn) S(LD_H_IND, nh + 2);
	p[n++] = (struct bpf_program_insn) J(JEQ_K, port, 0, 1);
	p[n++] = (struct bpf_program_insn) S(RET_K, 0xffff);
	p[n++] = (struct bpf_program_insn) S(RET_K, 0);
	return n;
}

// An IPv4/UDP packet after a link header of `lh` bytes.
static uint32_t make_udp(uint8_t* buf, uint32_t lh, int ether, uint16_t dport, uint32_t payload)
{
	memset(buf, 0, 256);
	if (ether)
	{
		buf[12] = 0x08;
		buf[13] = 0x00;
	}
	else
	{
		uint32_t af = 2;
		memcpy(buf, &af, 4);
	}
	uint8_t* ip = buf + lh;
	ip[0] = 0x45;
	ip[9] = 17;
	uint8_t* udp = ip + 20;
	udp[2] = (uint8_t)(dport >> 8);
	udp[3] = (uint8_t)dport;
	return lh + 20 + 8 + payload;
}

static void test_filters(void)
{
	struct bpf_program_insn prog[16];
	uint8_t pkt[256];

	for (int ether = 0; ether <= 1; ether++)
	{
		uint32_t lh = ether ? 14 : 4;
		uint32_t n = udp_port_filter(prog, lh, 5555, ether);
		CHECK(bpf_program_check(prog, n, 0x80000));

		uint32_t len = make_udp(pkt, lh, ether, 5555, 10);
		CHECK(bpf_program_run(prog, n, pkt, len, len) == 0xffff);

		len = make_udp(pkt, lh, ether, 5556, 10);
		CHECK(bpf_program_run(prog, n, pkt, len, len) == 0);

		// TCP is rejected.
		len = make_udp(pkt, lh, ether, 5555, 10);
		pkt[lh + 9] = 6;
		CHECK(bpf_program_run(prog, n, pkt, len, len) == 0);

		// A longer IP header moves the UDP header (LDX MSH).
		len = make_udp(pkt, lh, ether, 0, 10);
		pkt[lh] = 0x46;
		pkt[lh + 24 + 2] = 0x15;
		pkt[lh + 24 + 3] = 0xb3;
		CHECK(bpf_program_run(prog, n, pkt, len, len) == 0xffff);

		// Truncated capture: the port lies beyond the captured bytes.
		len = make_udp(pkt, lh, ether, 5555, 10);
		CHECK(bpf_program_run(prog, n, pkt, len, lh + 20 + 2) == 0);
	}
}

static void test_runtime(void)
{
	uint8_t pkt[8] = { 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0 };

	// Big-endian loads of each width.
	struct bpf_program_insn w[] = { S(LD_W_ABS, 2), S(RET_A, 0) };
	CHECK(bpf_program_run(w, N(w), pkt, 8, 8) == 0x56789abc);
	struct bpf_program_insn h[] = { S(LD_H_ABS, 6), S(RET_A, 0) };
	CHECK(bpf_program_run(h, N(h), pkt, 8, 8) == 0xdef0);

	// A load that ends past the captured data rejects the packet.
	struct bpf_program_insn oob[] = { S(LD_W_ABS, 5), S(RET_K, 1) };
	CHECK(bpf_program_run(oob, N(oob), pkt, 100, 8) == 0);

	// LEN is the original length, not the captured one.
	struct bpf_program_insn len[] = { S(LD_W_LEN, 0), S(RET_A, 0) };
	CHECK(bpf_program_run(len, N(len), pkt, 1500, 8) == 1500);

	// Indexed load whose offset wraps around 2^32.
	struct bpf_program_insn wrap[] = { S(LDX_IMM, 0xffffffff), S(LD_W_IND, 2), S(RET_K, 1) };
	CHECK(bpf_program_run(wrap, N(wrap), pkt, 8, 8) == 0);
	struct bpf_program_insn ind[] = { S(LDX_IMM, 2), S(LD_W_IND, 2), S(RET_A, 0) };
	CHECK(bpf_program_run(ind, N(ind), pkt, 8, 8) == 0x9abcdef0);

	// Division by a zero X rejects the packet.
	struct bpf_program_insn divx[] = { S(LD_IMM, 10), S(LDX_IMM, 0), S(ALU_DIV_X, 0), S(RET_K, 1) };
	CHECK(bpf_program_check(divx, N(divx), 0x80000));
	CHECK(bpf_program_run(divx, N(divx), pkt, 8, 8) == 0);

	// Arithmetic, scratch memory, TAX/TXA.
	struct bpf_program_insn alu[] = {
		S(LD_IMM, 7), S(ALU_MUL_K, 6), S(ALU_ADD_K, 8), // 50
		S(ST, 3), S(LDX_IMM, 5), S(ALU_SUB_X, 0),       // 45
		S(ALU_DIV_K, 2), S(ALU_LSH_K, 1),               // 44
		S(TAX, 0), S(LD_MEM, 3), S(STX, 4),             // A=50, M[4]=44
		S(ALU_AND_K, 0x32), S(TXA, 0), S(ALU_NEG, 0),   // A=-44
		S(RET_A, 0),
	};
	CHECK(bpf_program_check(alu, N(alu), 0x80000));
	CHECK(bpf_program_run(alu, N(alu), pkt, 8, 8) == (uint32_t)-44);

	// Shift counts are taken modulo 32.
	struct bpf_program_insn shr[] = { S(LD_IMM, 0x80000000), S(LDX_IMM, 33), S(ALU_RSH_X, 0), S(RET_A, 0) };
	CHECK(bpf_program_run(shr, N(shr), pkt, 8, 8) == 0x40000000);

	// Jumps: JSET, JGT, JEQ X and JA.
	struct bpf_program_insn jmp[] = {
		S(LD_B_ABS, 0),           // 0x12
		J(JSET_K, 0x02, 0, 5),    // taken
		J(JGT_K, 0x11, 0, 4),     // taken
		S(LDX_IMM, 0x12),
		J(JEQ_X, 0, 0, 2),        // taken
		J(JA, 1, 0, 0),
		S(RET_K, 0),
		S(RET_K, 77),
	};
	CHECK(bpf_program_check(jmp, N(jmp), 0x80000));
	CHECK(bpf_program_run(jmp, N(jmp), pkt, 8, 8) == 77);

	// The returned value is the snapshot length.
	struct bpf_program_insn snap[] = { S(RET_K, 3) };
	CHECK(bpf_program_run(snap, N(snap), pkt, 8, 8) == 3);
}

static void test_checker(void)
{
	struct bpf_program_insn ret[] = { S(RET_K, 1) };
	CHECK(bpf_program_check(ret, 1, 0x80000));
	CHECK(!bpf_program_check(ret, 0, 0x80000));
	CHECK(!bpf_program_check(NULL, 1, 0x80000));

	static struct bpf_program_insn big[BPF_PROGRAM_MAX_INSNS + 1];
	for (int i = 0; i < BPF_PROGRAM_MAX_INSNS + 1; i++)
		big[i] = (struct bpf_program_insn) S(RET_K, 1);
	CHECK(bpf_program_check(big, BPF_PROGRAM_MAX_INSNS, 0x80000));
	CHECK(!bpf_program_check(big, BPF_PROGRAM_MAX_INSNS + 1, 0x80000));

	// Must end with a return.
	struct bpf_program_insn noret[] = { S(RET_K, 1), S(LD_IMM, 0) };
	CHECK(!bpf_program_check(noret, N(noret), 0x80000));

	// Jumps past the end.
	struct bpf_program_insn ja[] = { J(JA, 1, 0, 0), S(RET_K, 1) };
	CHECK(!bpf_program_check(ja, N(ja), 0x80000));
	struct bpf_program_insn ja_wrap[] = { J(JA, 0xffffffff, 0, 0), S(RET_K, 1) };
	CHECK(!bpf_program_check(ja_wrap, N(ja_wrap), 0x80000));
	struct bpf_program_insn jt[] = { J(JEQ_K, 0, 1, 0), S(RET_K, 1) };
	CHECK(!bpf_program_check(jt, N(jt), 0x80000));
	struct bpf_program_insn jf[] = { J(JEQ_K, 0, 0, 1), S(RET_K, 1) };
	CHECK(!bpf_program_check(jf, N(jf), 0x80000));

	// Scratch memory indices.
	struct bpf_program_insn st[] = { S(ST, 16), S(RET_K, 1) };
	CHECK(!bpf_program_check(st, N(st), 0x80000));
	struct bpf_program_insn ldm[] = { S(LD_MEM, 16), S(RET_K, 1) };
	CHECK(!bpf_program_check(ldm, N(ldm), 0x80000));
	struct bpf_program_insn ldm_ok[] = { S(LD_MEM, 15), S(RET_K, 1) };
	CHECK(bpf_program_check(ldm_ok, N(ldm_ok), 0x80000));

	// Division by a constant zero.
	struct bpf_program_insn div0[] = { S(ALU_DIV_K, 0), S(RET_K, 1) };
	CHECK(!bpf_program_check(div0, N(div0), 0x80000));

	// Darwin's instruction set has no modulo or xor.
	struct bpf_program_insn mod[] = { S(ALU_MOD_K, 3), S(RET_K, 1) };
	CHECK(!bpf_program_check(mod, N(mod), 0x80000));

	// Packet offsets beyond the largest buffer.
	struct bpf_program_insn far[] = { S(LD_W_ABS, 0x80000), S(RET_K, 1) };
	CHECK(!bpf_program_check(far, N(far), 0x80000));
	far[0].k = 0x7ffff;
	CHECK(bpf_program_check(far, N(far), 0x80000));

	// Unknown load mode.
	struct bpf_program_insn mode[] = { S(0xe0, 0), S(RET_K, 1) };
	CHECK(!bpf_program_check(mode, N(mode), 0x80000));
}

int main(void)
{
	test_filters();
	test_runtime();
	test_checker();

	if (failures)
	{
		fprintf(stderr, "%d failure(s)\n", failures);
		return 1;
	}
	puts("PASS bpf program checker and interpreter");
	return 0;
}
