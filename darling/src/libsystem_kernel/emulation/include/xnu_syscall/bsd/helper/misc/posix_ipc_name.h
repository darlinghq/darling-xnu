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

#ifndef _POSIX_IPC_NAME_H
#define _POSIX_IPC_NAME_H

#include <stddef.h>

// Names of POSIX shared memory objects and named semaphores as the guest
// passes them, translated to names the host's shm_open()/sem_open() accept.
// See posix_ipc_name.c.

// Longest name (in bytes, leading slash included) shm_open() accepts, and
// the longest sem_open()/sem_unlink() accept.
#define POSIX_IPC_SHM_NAME_MAX 31
#define POSIX_IPC_SEM_NAME_MAX 30

// Upper bound of the translated name, terminator included, for a guest name
// of up to POSIX_IPC_SHM_NAME_MAX bytes.
#define POSIX_IPC_HOST_NAME_SIZE 100

// Translates the guest name into the host name. Returns 0, or a positive
// Darwin errno: EFAULT for a NULL name, ENAMETOOLONG if the name is longer
// than max_len.
int posix_ipc_name_to_host(const char* name, size_t max_len, char* host, size_t host_size);

#endif // _POSIX_IPC_NAME_H
