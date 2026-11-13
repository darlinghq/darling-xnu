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

#ifndef _HOST_PLATFORM_H
#define _HOST_PLATFORM_H

#include <stdbool.h>

// Whether the host Darling runs on is Android (Termux and the like).
//
// This is the one place the guest asks about it, so that every host-specific
// workaround for Android hangs off a single, easily found and removable test.
// The answer is computed once per process: it is whether the dynamic loader
// the host ELF process (mldr) was started with is Bionic's linker, taken from
// the process's own program headers. On a glibc or musl host it is false.
bool host_platform_is_android(void);

#endif // _HOST_PLATFORM_H
