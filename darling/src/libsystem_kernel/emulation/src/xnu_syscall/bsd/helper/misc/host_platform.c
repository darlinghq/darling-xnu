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

#include <darling/emulation/xnu_syscall/bsd/helper/misc/host_platform.h>

#include <darling/emulation/conversion/common_at.h>
#include <darling/emulation/conversion/fcntl/open.h>

#include <stddef.h>
#include <stdint.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

// Values from the ELF specification and the Linux auxiliary vector (auxv(3)).
#define AT_NULL_ 0
#define AT_PHDR_ 3
#define AT_PHENT_ 4
#define AT_PHNUM_ 5
#define PT_INTERP_ 3
#define PT_PHDR_ 6

#if defined(__LP64__)
struct elf_phdr_min {
	uint32_t p_type;
	uint32_t p_flags;
	uint64_t p_offset;
	uint64_t p_vaddr;
	uint64_t p_paddr;
	uint64_t p_filesz;
	uint64_t p_memsz;
	uint64_t p_align;
};
typedef uint64_t auxv_word;
#else
struct elf_phdr_min {
	uint32_t p_type;
	uint32_t p_offset;
	uint32_t p_vaddr;
	uint32_t p_paddr;
	uint32_t p_filesz;
	uint32_t p_memsz;
	uint32_t p_flags;
	uint32_t p_align;
};
typedef uint32_t auxv_word;
#endif

// Android's dynamic loader is /system/bin/linker (32-bit) or linker64, with
// sanitizer variants named linker_asan and so on; glibc's is ld-linux*.so*
// and musl's ld-musl*.so*.
static bool is_bionic_linker_path(const char* path, size_t max)
{
	size_t base = 0;
	size_t i;

	for (i = 0; i < max && path[i]; i++)
		if (path[i] == '/')
			base = i + 1;
	if (i == max)
		return false;

	static const char prefix[] = "linker";
	for (i = 0; i < sizeof(prefix) - 1; i++)
		if (path[base + i] != prefix[i])
			return false;
	return true;
}

static bool detect_android(void)
{
	auxv_word phdr = 0, phent = 0, phnum = 0;
	auxv_word pair[2];
	bool found = false;

	int fd = LINUX_SYSCALL(__NR_openat, LINUX_AT_FDCWD, "/proc/self/auxv", LINUX_O_RDONLY | LINUX_O_CLOEXEC, 0);
	if (fd < 0)
		return false;

	while (LINUX_SYSCALL(__NR_read, fd, pair, sizeof(pair)) == (long) sizeof(pair) && pair[0] != AT_NULL_)
	{
		if (pair[0] == AT_PHDR_)
			phdr = pair[1];
		else if (pair[0] == AT_PHENT_)
			phent = pair[1];
		else if (pair[0] == AT_PHNUM_)
			phnum = pair[1];
	}
	LINUX_SYSCALL(__NR_close, fd);

	if (phdr == 0 || phent != sizeof(struct elf_phdr_min) || phnum == 0)
		return false;

	// The kernel maps the executable's program headers at phdr and the auxv
	// values are those of this very process, so the table and the interpreter
	// path it names are valid memory. The load bias is the difference between
	// where the PT_PHDR entry says the table is and where it is.
	const struct elf_phdr_min* table = (const struct elf_phdr_min*) (uintptr_t) phdr;
	uintptr_t bias = 0;
	bool have_bias = false;
	for (auxv_word i = 0; i < phnum; i++)
	{
		if (table[i].p_type == PT_PHDR_)
		{
			bias = (uintptr_t) phdr - (uintptr_t) table[i].p_vaddr;
			have_bias = true;
			break;
		}
	}
	if (!have_bias)
		return false;

	for (auxv_word i = 0; i < phnum; i++)
	{
		if (table[i].p_type == PT_INTERP_ && table[i].p_filesz > 0 && table[i].p_filesz <= 4096)
		{
			found = is_bionic_linker_path((const char*) (bias + (uintptr_t) table[i].p_vaddr), (size_t) table[i].p_filesz);
			break;
		}
	}
	return found;
}

bool host_platform_is_android(void)
{
	// 0: not determined yet, 1: no, 2: yes. Threads that find it undetermined
	// each compute the same answer, so a race only repeats the work.
	static int state;
	int current = __atomic_load_n(&state, __ATOMIC_ACQUIRE);

	if (current == 0)
	{
		current = detect_android() ? 2 : 1;
		__atomic_store_n(&state, current, __ATOMIC_RELEASE);
	}
	return current == 2;
}
