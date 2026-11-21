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

#include <darling/emulation/linux_premigration/ext/for-iosurface.h>

#include <sys/fcntl.h>

#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/xnu_syscall/bsd/impl/fcntl/fcntl.h>

#include <darlingserver/rpc.h>

// darlingserver calls return 0 or a negative Linux errno value
static int to_bsd_error(int result) {
	return (result < 0) ? -errno_linux_to_bsd(result) : 0;
};

// like XNU's descriptors for kernel objects, these are closed on exec
static int received(int fd) {
	if (fd >= 0)
		sys_fcntl_nocancel(fd, F_SETFD, FD_CLOEXEC);
	return fd;
};

int __darling_iosurface_retain(int memory, uint32_t id_hint, uint32_t flags, uint32_t counter_offset, uint32_t* surface_id) {
	uint32_t value = 0;
	int result = to_bsd_error(dserver_rpc_iosurface_retain(memory, id_hint, flags, counter_offset, &value));

	if (result == 0)
		*surface_id = value;
	return result;
};

int __darling_iosurface_release(uint32_t surface_id) {
	return to_bsd_error(dserver_rpc_iosurface_release(surface_id));
};

int __darling_iosurface_lookup(uint32_t surface_id, int* memory) {
	int fd = -1;
	int result = to_bsd_error(dserver_rpc_iosurface_lookup(surface_id, &fd));

	if (result == 0)
		*memory = received(fd);
	return result;
};

int __darling_iosurface_publish_gpu_buffer(uint32_t surface_id, int buffer, dserver_iosurface_gpu_buffer_t* info, int* published) {
	int fd = -1;
	int result = to_bsd_error(dserver_rpc_iosurface_publish_gpu_buffer(surface_id, buffer, info, sizeof(*info), &fd));

	if (result == 0)
		*published = received(fd);
	return result;
};

int __darling_iosurface_copy_gpu_buffer(uint32_t surface_id, dserver_iosurface_gpu_buffer_t* info, int* buffer) {
	int fd = -1;
	int result = to_bsd_error(dserver_rpc_iosurface_copy_gpu_buffer(surface_id, info, sizeof(*info), &fd));

	if (result == 0)
		*buffer = received(fd);
	return result;
};

int __darling_iosurface_set_usage(uint32_t surface_id, uint32_t usage) {
	return to_bsd_error(dserver_rpc_iosurface_set_usage(surface_id, usage));
};

int __darling_iosurface_get_state(uint32_t surface_id, uint32_t* holders, uint32_t* memory_users, uint32_t* flags) {
	uint32_t h = 0, m = 0, f = 0;
	int result = to_bsd_error(dserver_rpc_iosurface_get_state(surface_id, &h, &m, &f));

	if (result == 0) {
		*holders = h;
		*memory_users = m;
		*flags = f;
	}
	return result;
};
