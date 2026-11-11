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

// Compile-only test: code built against Darling's SDK the way a third-party
// project builds it (no -DDARLING, no -DPLATFORM_*), including with a Linux
// compiler target that spells arm64 as __aarch64__. See sdk-header-consumers.md.

#if defined(__aarch64__) && !defined(__arm64__)

// A Linux compiler: only the machine dispatchers are expected to work, the
// arm/ headers behind most of them still test __arm64__ themselves.
#include <machine/_limits.h>
#include <machine/_types.h>
#include <machine/_param.h>
#include <machine/param.h>
#include <mach/machine/kern_return.h>
#include <mach/machine/exception.h>

_Static_assert(sizeof(__darwin_size_t) == sizeof(void*), "__darwin_size_t");
_Static_assert(sizeof(kern_return_t) == 4, "kern_return_t");

#else

#include <sys/cdefs.h>
#include <sys/types.h>
#include <machine/_types.h>
#include <machine/_limits.h>
#include <machine/_param.h>
#include <machine/endian.h>
#include <machine/limits.h>
#include <machine/param.h>
#include <machine/signal.h>
#include <machine/types.h>
#include <mach/machine/kern_return.h>
#include <mach/machine/vm_types.h>
#include <mach/machine/thread_status.h>
#include <mach/machine/vm_param.h>
#include <mach/machine/exception.h>
#include <stdatomic.h>
#include <os/trace_base.h>
#include <os/trace.h>

// cdefs.h must pick the macOS conformance settings without a PLATFORM_ macro.
#ifndef __DARWIN_ONLY_UNIX_CONFORMANCE
#error "__DARWIN_ONLY_UNIX_CONFORMANCE is not defined without -DPLATFORM_MacOSX"
#endif
#if defined(__arm64__) || defined(__aarch64__) || defined(__x86_64__)
_Static_assert(__DARWIN_ONLY_UNIX_CONFORMANCE == 1, "64-bit macOS is UNIX03-only");
#endif

#define STR2(x) #x
#define STR(x) STR2(x)
#if defined(__arm64__) || defined(__aarch64__) || defined(__x86_64__)
// No $UNIX2003 suffixes on 64-bit macOS.
_Static_assert(sizeof(STR(__DARWIN_SUF_UNIX03)) == 1, "unexpected __DARWIN_SUF_UNIX03");
#endif

// The machine/ dispatchers routed to the right architecture.
_Static_assert(sizeof(__darwin_size_t) == sizeof(void*), "__darwin_size_t");
_Static_assert(sizeof(kern_return_t) == 4, "kern_return_t");
_Static_assert(sizeof(vm_offset_t) == sizeof(void*), "vm_offset_t");
_Static_assert(sizeof(natural_t) == 4, "natural_t");

// stdatomic.h must provide the C11 atomics, also when it defers to Clang's.
static atomic_int counter;

int sdk_header_consumers_check(void)
{
	atomic_fetch_add_explicit(&counter, 1, memory_order_relaxed);
	return atomic_load(&counter);
}

#endif
