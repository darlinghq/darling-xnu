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

#ifndef _DARLING_EMULATION_PLATFORM_IDENTITY_H_
#define _DARLING_EMULATION_PLATFORM_IDENTITY_H_

#include <darlingserver/rpc-supplement.h>

/**
 * The platform identity (see `dserver_platform_identity_t`) as darlingserver reported it the first
 * time this process asked. The values stay the same for the lifetime of the process (and of its
 * forked children), even if a new identity is pushed in the meantime.
 *
 * @returns `NULL` if darlingserver could not be asked.
 */
const dserver_platform_identity_t* platform_identity(void);

#endif // _DARLING_EMULATION_PLATFORM_IDENTITY_H_
