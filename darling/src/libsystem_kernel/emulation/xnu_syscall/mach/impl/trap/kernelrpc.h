#ifndef DARLING_MACH_EMULATION_KERNELRPC_H
#define DARLING_MACH_EMULATION_KERNELRPC_H

#include <mach/kern_return.h>
#include <mach/mach_traps.h>

kern_return_t _kernelrpc_mach_port_allocate_trap_impl(mach_port_name_t target, mach_port_right_t right, mach_port_name_t *name);
kern_return_t _kernelrpc_mach_port_construct_trap_impl(mach_port_name_t target, mach_port_options_t *options, uint64_t context, mach_port_name_t *name);
kern_return_t _kernelrpc_mach_port_deallocate_trap_impl(mach_port_name_t target, mach_port_name_t name);
kern_return_t _kernelrpc_mach_port_destroy_trap_impl(mach_port_name_t target, mach_port_name_t name);
kern_return_t _kernelrpc_mach_port_destruct_trap_impl(mach_port_name_t target, mach_port_name_t name, mach_port_delta_t srdelta, uint64_t guard);
kern_return_t _kernelrpc_mach_port_extract_member_trap_impl(mach_port_name_t target, mach_port_name_t name, mach_port_name_t pset);
kern_return_t _kernelrpc_mach_port_get_attributes_impl(mach_port_name_t target, mach_port_name_t name, mach_port_flavor_t flavor, mach_port_info_t port_info_out, mach_msg_type_number_t* port_info_outCnt);
kern_return_t _kernelrpc_mach_port_guard_trap_impl(mach_port_name_t target, mach_port_name_t name, uint64_t guard, boolean_t strict);
kern_return_t _kernelrpc_mach_port_insert_member_trap_impl(mach_port_name_t target, mach_port_name_t name, mach_port_name_t pset);
kern_return_t _kernelrpc_mach_port_insert_right_trap_impl(mach_port_name_t target, mach_port_name_t name, mach_port_name_t poly, mach_msg_type_name_t polyPoly);
kern_return_t _kernelrpc_mach_port_mod_refs_trap_impl(mach_port_name_t target, mach_port_name_t name, mach_port_right_t right, mach_port_delta_t delta);
kern_return_t _kernelrpc_mach_port_move_member_trap_impl(mach_port_name_t target, mach_port_name_t member, mach_port_name_t after);
kern_return_t _kernelrpc_mach_port_request_notification_impl(ipc_space_t task, mach_port_name_t name, mach_msg_id_t msgid, mach_port_mscount_t sync, mach_port_name_t notify, mach_msg_type_name_t notifyPoly, mach_port_name_t* previous);
kern_return_t _kernelrpc_mach_port_type_impl(ipc_space_t task, mach_port_name_t name, mach_port_type_t* ptype);
kern_return_t _kernelrpc_mach_port_unguard_trap_impl(mach_port_name_t target, mach_port_name_t name, uint64_t guard);
kern_return_t _kernelrpc_mach_vm_allocate_trap_impl(mach_port_name_t target, mach_vm_offset_t *addr, mach_vm_size_t size, int flags);
kern_return_t _kernelrpc_mach_vm_deallocate_trap_impl(mach_port_name_t target, mach_vm_address_t address, mach_vm_size_t size);
kern_return_t _kernelrpc_mach_vm_map_trap_impl(mach_port_name_t target, mach_vm_offset_t *address, mach_vm_size_t size, mach_vm_offset_t mask, int flags, vm_prot_t cur_protection);
kern_return_t _kernelrpc_mach_vm_protect_trap_impl(mach_port_name_t target, mach_vm_address_t address, mach_vm_size_t size, boolean_t set_maximum, vm_prot_t new_protection);

#endif // DARLING_MACH_EMULATION_KERNELRPC_H
