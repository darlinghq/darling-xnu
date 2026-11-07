/**
 * This file is part of Darling.
 *
 * Copyright (C) 2026 Darling Developers
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

#ifndef _DARLING_EMULATION_DC_MSEAL_H_
#define _DARLING_EMULATION_DC_MSEAL_H_

/**
 * Darling-private helper for dyld (not part of the macOS API, and not
 * exported from libsystem_kernel.dylib: it is reached only by code that
 * links the static libsystem_kernel, as dyld does).
 *
 * Makes a read-only mapping immutable. The pages in [addr, addr + len) are
 * copied into anonymous memory that replaces the mapping at the same
 * address, protected with `prot`, and then sealed with Linux mseal(2): the
 * range can no longer be unmapped, remapped or have its protection changed.
 * Copying first matters for file mappings, because sealing alone does not
 * stop the file itself from changing underneath.
 *
 * `addr` must be page-aligned (the runtime page size) and `len` non-zero;
 * `prot` must include PROT_READ and not PROT_WRITE. Returns 0, or a
 * negative BSD errno value: EINVAL for bad arguments, and the errors of
 * mmap(), mprotect(), mremap() and mseal() otherwise (ENOSYS on Linux
 * kernels without mseal, which leaves the copy in place, unsealed).
 */
__attribute__((visibility("hidden")))
int __darling_mseal_range(void* addr, unsigned long len, int prot);

#endif // _DARLING_EMULATION_DC_MSEAL_H_
