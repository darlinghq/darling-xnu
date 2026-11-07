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

#define __SYSCALL_32BIT_ARG_BYTES 0
#include "SYS.h"

#ifndef SYS_geteuid
#error "SYS_geteuid not defined. The header files libsyscall is building against do not match syscalls.master."
#endif

/* geteuid() always succeeds; see __SYSCALL_NOERROR in SYS.h. */
#if defined(__i386__) && defined(DARLING)
__SYSCALL_NOERROR(___geteuid, geteuid, 0)
#elif defined(__arm64__)
MI_ENTRY_POINT(___geteuid)
SYSCALL_NONAME(geteuid, 0, cerror_nocancel)
ret
#else
__SYSCALL2(___geteuid, geteuid, 0, cerror_nocancel)
#endif
#if defined(__arm64__)
	.globl	_geteuid
	.set	_geteuid, ___geteuid
#endif

#if defined(__i386__)
	.globl	_geteuid
	.set	_geteuid, ___geteuid
#endif

#if defined(__x86_64__)
	.globl	_geteuid
	.set	_geteuid, ___geteuid
#endif

#if defined(__ppc__)
	.globl	_geteuid
	.set	_geteuid, ___geteuid
#endif
