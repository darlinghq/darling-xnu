#include "kernelrpc.h"

// TODO: To make it eaiser to port this code into the Linux side (when we implement
// syscall user dispatch support in mldr), we should avoid directly using macOS headers.
// In other words, `BSD_MAP_ANON` instead of `MAP_ANON`

#include <sys/mman.h>

#include <darlingserver/rpc.h>

#include <emulation/common/simple.h>
#include <emulation/linux_api/c_api/mremap.h>
#include <emulation/xnu_syscall/bsd/bsd_syscall_table.h>
#include <emulation/xnu_syscall/mach/impl/mach_other.h>

kern_return_t _kernelrpc_mach_port_allocate_trap_impl(
    mach_port_name_t target, mach_port_right_t right, mach_port_name_t *name
) {
	int code = dserver_rpc_mach_port_allocate(target, right, name);

	if (code < 0) {
		__simple_printf("mach_port_allocate failed (internally): %d\n", code);
		__simple_abort();
	}

	return code;
}

kern_return_t _kernelrpc_mach_port_construct_trap_impl(
    mach_port_name_t target, mach_port_options_t *options, uint64_t context,
    mach_port_name_t *name
) {
	int code = dserver_rpc_mach_port_construct(target, options, context, name);

	if (code < 0) {
		__simple_printf("mach_port_construct failed (internally): %d\n", code);
		__simple_abort();
	}

	return code;
}

kern_return_t _kernelrpc_mach_port_deallocate_trap_impl(
    mach_port_name_t target, mach_port_name_t name
) {
	int code = dserver_rpc_mach_port_deallocate(target, name);

	if (code < 0) {
		__simple_printf("mach_port_deallocate failed (internally): %d\n", code);
		__simple_abort();
	}

	return code;
}

kern_return_t _kernelrpc_mach_port_destroy_trap_impl(
    mach_port_name_t target, mach_port_name_t name
) {
	int code = dserver_rpc_mach_port_destruct(target, name, 0, 0);

	if (code < 0) {
		__simple_printf("mach_port_destroy failed (internally): %d\n", code);
		__simple_abort();
	}

	return code;
}

kern_return_t _kernelrpc_mach_port_destruct_trap_impl(
    mach_port_name_t target, mach_port_name_t name, mach_port_delta_t srdelta,
    uint64_t guard
) {
	int code = dserver_rpc_mach_port_destruct(target, name, srdelta, guard);

	if (code < 0) {
		__simple_printf("mach_port_destruct failed (internally): %d\n", code);
		__simple_abort();
	}

	return code;
}

kern_return_t _kernelrpc_mach_port_extract_member_trap_impl(
    mach_port_name_t target, mach_port_name_t name, mach_port_name_t pset
) {
	int code = dserver_rpc_mach_port_extract_member(target, name, pset);

	if (code < 0) {
		__simple_printf("mach_port_extract_member failed (internally): %d\n", code);
		__simple_abort();
	}

	return code;
}

kern_return_t _kernelrpc_mach_port_get_attributes_impl(
    mach_port_name_t target, mach_port_name_t name, mach_port_flavor_t flavor,
    mach_port_info_t port_info_out, mach_msg_type_number_t* port_info_outCnt
) {
	int code = dserver_rpc_mach_port_get_attributes(target, name, flavor, port_info_out, port_info_outCnt);

	if (code < 0) {
		__simple_printf("mach_port_get_attributes failed (internally): %d\n", code);
		__simple_abort();
	}

	return code;
}

kern_return_t _kernelrpc_mach_port_guard_trap_impl(
    mach_port_name_t target, mach_port_name_t name, uint64_t guard,
    boolean_t strict
) {
	int code = dserver_rpc_mach_port_guard(target, name, guard, strict);

	if (code < 0) {
		__simple_printf("mach_port_guard failed (internally): %d\n", code);
		__simple_abort();
	}

	return code;
}

kern_return_t _kernelrpc_mach_port_insert_member_trap_impl(
    mach_port_name_t target, mach_port_name_t name, mach_port_name_t pset
) {
	int code = dserver_rpc_mach_port_insert_member(target, name, pset);

	if (code < 0) {
		__simple_printf("mach_port_insert_member failed (internally): %d\n", code);
		__simple_abort();
	}

	return code;
}

kern_return_t _kernelrpc_mach_port_insert_right_trap_impl(
    mach_port_name_t target, mach_port_name_t name, mach_port_name_t poly,
    mach_msg_type_name_t polyPoly
) {
	int code = dserver_rpc_mach_port_insert_right(target, name, poly, polyPoly);

	if (code < 0) {
		__simple_printf("mach_port_insert_right failed (internally): %d\n", code);
		__simple_abort();
	}

	return code;
}

kern_return_t _kernelrpc_mach_port_mod_refs_trap_impl(
    mach_port_name_t target, mach_port_name_t name, mach_port_right_t right,
    mach_port_delta_t delta
) {
	int code = dserver_rpc_mach_port_mod_refs(target, name, right, delta);

	if (code < 0) {
		__simple_printf("mach_port_deallocate failed (internally): %d\n", code);
		__simple_abort();
	}

	return code;
}

kern_return_t _kernelrpc_mach_port_move_member_trap_impl(
    mach_port_name_t target, mach_port_name_t member, mach_port_name_t after
) {
	int code = dserver_rpc_mach_port_move_member(target, member, after);

	if (code < 0) {
		__simple_printf("mach_port_move_member failed (internally): %d\n", code);
		__simple_abort();
	}

	return code;
}

kern_return_t _kernelrpc_mach_port_request_notification_impl(
    ipc_space_t task, mach_port_name_t name, mach_msg_id_t msgid,
    mach_port_mscount_t sync, mach_port_name_t notify, mach_msg_type_name_t notifyPoly,
    mach_port_name_t* previous
) {
	int code = dserver_rpc_mach_port_request_notification(task, name, msgid, sync, notify, notifyPoly, previous);

	if (code < 0) {
		__simple_printf("mach_port_request_notification failed (internally): %d\n", code);
		__simple_abort();
	}

	return code;
}

kern_return_t _kernelrpc_mach_port_type_impl(
    ipc_space_t task, mach_port_name_t name, mach_port_type_t* ptype
) {
	int code = dserver_rpc_mach_port_type(task, name, ptype);

	if (code < 0) {
		__simple_printf("mach_port_type failed (internally): %d\n", code);
		__simple_abort();
	}

	return code;
}

kern_return_t _kernelrpc_mach_port_unguard_trap_impl(
    mach_port_name_t target, mach_port_name_t name, uint64_t guard
) {
	int code = dserver_rpc_mach_port_unguard(target, name, guard);

	if (code < 0) {
		__simple_printf("mach_port_unguard failed (internally): %d\n", code);
		__simple_abort();
	}

	return code;
}

kern_return_t _kernelrpc_mach_vm_allocate_trap_impl(
    mach_port_name_t target, mach_vm_offset_t *addr, mach_vm_size_t size,
    int flags
) {
	if (target != 0 && target != task_self_trap_impl())
	{
		int code = dserver_rpc_mach_vm_allocate(target, addr, size, flags);

		if (code < 0) {
			__simple_printf("mach_vm_allocate failed (internally): %d\n", code);
			__simple_abort();
		}

		return code;
	}
	else
	{
		return _kernelrpc_mach_vm_map_trap_impl(target, addr,
				size, 0, flags, VM_PROT_READ | VM_PROT_WRITE);
	}
}

kern_return_t _kernelrpc_mach_vm_deallocate_trap_impl(
    mach_port_name_t target, mach_vm_address_t address, mach_vm_size_t size
) {
	int ret;

	if (target != 0 && target != task_self_trap_impl())
	{
		int code = dserver_rpc_mach_vm_deallocate(target, address, size);

		if (code < 0) {
			__simple_printf("mach_vm_deallocate failed (internally): %d\n", code);
			__simple_abort();
		}

		return code;
	}
	else
	{
		// XNU returns KERN_INVALID_ARGUMENT for address overflow, 
		// and allows NULL address if size is 0.
		if (address + size < address)
		{
			return KERN_INVALID_ARGUMENT;
		}

		if (size == (mach_vm_offset_t)0)
		{
			return KERN_SUCCESS;
		}

		ret = sys_munmap((void*)address, size);

		if (ret == -1)
			return KERN_FAILURE;

		return KERN_SUCCESS;
	}
}

kern_return_t _kernelrpc_mach_vm_map_trap_impl(
    mach_port_name_t target, mach_vm_offset_t *address, mach_vm_size_t size,
    mach_vm_offset_t mask, int flags, vm_prot_t cur_protection
) {
	// We cannot allocate memory in other processes
	if (target != 0 && target != task_self_trap_impl())
		return KERN_FAILURE;

	void* addr;
	int prot = 0;
	int posix_flags = MAP_ANON | MAP_PRIVATE;

	if (cur_protection & VM_PROT_READ)
		prot |= PROT_READ;
	if (cur_protection & VM_PROT_WRITE)
		prot |= PROT_WRITE;
	if (cur_protection & VM_PROT_EXECUTE)
		prot |= PROT_EXEC;

	if (!(flags & VM_FLAGS_ANYWHERE))
		posix_flags |= MAP_FIXED;
	if ((flags >> 24) == VM_MEMORY_REALLOC)
		addr = (void*)__linux_mremap(((char*)*address) - 0x1000, 0x1000, 0x1000 + size, 0, NULL);
	else
		addr = sys_mmap((void*)*address, size, prot, posix_flags, -1, 0);
	
	if (addr == MAP_FAILED)
	{
		return KERN_FAILURE;
	}
	
	if (mask && ( ((uintptr_t)addr) & mask) != 0)
	{
		uintptr_t boundary, q, diff, iaddr;
		
		// Alignment was requested, but we couldn't get it the easy way
		sys_munmap(addr, size);
		
		// This may not work for some crazy masks. Consider using __builtin_clz().
		boundary = mask + 1;
		
		iaddr = (uintptr_t)sys_mmap((void*)*address, size + boundary, prot, posix_flags, -1, 0);
		if (iaddr == (uintptr_t) MAP_FAILED)
			return KERN_FAILURE;
		
		q = (iaddr + (boundary-1)) / boundary * boundary;
		diff = q - iaddr;
		
		if (diff > 0)
			sys_munmap((void*)iaddr, diff);
		if (boundary - diff > 0)
			sys_munmap((void*) (q + size), boundary - diff);
		
		addr = (void*) q;
	}

	*address = (uintptr_t)addr;
	return KERN_SUCCESS;
}

kern_return_t _kernelrpc_mach_vm_protect_trap_impl(
    mach_port_name_t target, mach_vm_address_t address, mach_vm_size_t size,
    boolean_t set_maximum, vm_prot_t new_protection
) {
	int prot = 0;
	int ret;

	if (target != 0 && target != task_self_trap_impl())
		return MACH_SEND_INVALID_DEST;

	if (new_protection & VM_PROT_READ)
		prot |= PROT_READ;
	if (new_protection & VM_PROT_WRITE)
		prot |= PROT_WRITE;
	if (new_protection & VM_PROT_EXECUTE)
		prot |= PROT_EXEC;

	ret = sys_mprotect((void*)address, size, prot);
	if (ret == -1)
		return KERN_FAILURE;

	return KERN_SUCCESS;
}