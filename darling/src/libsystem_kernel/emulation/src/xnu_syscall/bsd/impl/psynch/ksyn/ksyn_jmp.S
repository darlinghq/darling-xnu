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

// ksyn_setjmp() and ksyn_longjmp() for arm64, which has no __builtin_setjmp (see KSYN_SETJMP in
// ksyn_thread.h). The buffer (ksyn_jmp_buf) holds the registers a called function must preserve
// under the AArch64 procedure call standard: x19-x28, the frame pointer x29, the link register
// x30, sp and d8-d15. x18 is the platform register and is left alone.

#if defined(__arm64__) || defined(__aarch64__)

.text
.p2align 2

// int ksyn_setjmp(ksyn_jmp_buf buf): 0 when called, 1 when ksyn_longjmp() comes back to it
.globl _ksyn_setjmp
.private_extern _ksyn_setjmp
_ksyn_setjmp:
	stp	x19, x20, [x0, #0]
	stp	x21, x22, [x0, #16]
	stp	x23, x24, [x0, #32]
	stp	x25, x26, [x0, #48]
	stp	x27, x28, [x0, #64]
	stp	x29, x30, [x0, #80]
	mov	x9, sp
	str	x9, [x0, #96]
	stp	d8, d9, [x0, #104]
	stp	d10, d11, [x0, #120]
	stp	d12, d13, [x0, #136]
	stp	d14, d15, [x0, #152]
	mov	w0, #0
	ret

// void ksyn_longjmp(ksyn_jmp_buf buf): returns from the ksyn_setjmp() call that filled buf, with 1
.globl _ksyn_longjmp
.private_extern _ksyn_longjmp
_ksyn_longjmp:
	ldp	x19, x20, [x0, #0]
	ldp	x21, x22, [x0, #16]
	ldp	x23, x24, [x0, #32]
	ldp	x25, x26, [x0, #48]
	ldp	x27, x28, [x0, #64]
	ldp	x29, x30, [x0, #80]
	ldr	x9, [x0, #96]
	mov	sp, x9
	ldp	d8, d9, [x0, #104]
	ldp	d10, d11, [x0, #120]
	ldp	d12, d13, [x0, #136]
	ldp	d14, d15, [x0, #152]
	mov	w0, #1
	ret

#endif
