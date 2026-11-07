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

#ifndef _DARLING_EMULATION_FOR_SYSCONFIG_H_
#define _DARLING_EMULATION_FOR_SYSCONFIG_H_

#include <darling/emulation/common/base.h>
#include <darlingserver/rpc-supplement.h>

CPP_EXTERN_BEGIN

// Darling-private access to the system configuration that darlingserver holds
// (see `dserver_system_config_t`). Each returns 0 or a BSD errno value.

/**
 * Copies the configuration in effect into @p config.
 */
VISIBLE
int __darling_get_system_config(dserver_system_config_t* config);

/**
 * Pushes a new configuration. Only root may do this (`EPERM` otherwise);
 * a malformed configuration fails with `EINVAL`.
 */
VISIBLE
int __darling_set_system_config(const dserver_system_config_t* config);

/**
 * Waits until the first configuration has been pushed. Fails with `EINTR`
 * when interrupted by a signal; the caller may simply call it again.
 */
VISIBLE
int __darling_wait_system_config(void);

CPP_EXTERN_END

#endif // _DARLING_EMULATION_FOR_SYSCONFIG_H_
