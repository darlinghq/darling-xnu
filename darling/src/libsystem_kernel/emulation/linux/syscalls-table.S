.text
.globl __darling_bsd_syscall
.private_extern __darling_bsd_syscall

// To better understand how XTRACE_RESERVED_SPACE
// works, I recommend looking at `src/xtrace/xtracelib.c`
// for details.
#if defined(__x86_64__)
.macro XTRACE_RESERVED_SPACE
	.space 13, 0x90
.endmacro

#elif defined( __i386__)
.macro XTRACE_RESERVED_SPACE
	.space 7, 0x90
.endmacro

#elif defined(__arm64__)
.macro XTRACE_RESERVED_SPACE
	nop ; movk
	nop ; movk
	nop ; movk
	nop ; movk
	nop ; blr
.endmacro

#endif

#if defined(__x86_64__)

__darling_bsd_syscall:
Lentry_hook:
	XTRACE_RESERVED_SPACE
	movq    ___bsd_syscall_table@GOTPCREL(%rip), %r10
	movq	(%r10,%rax,8), %r10
	test	%r10, %r10
	jz		.no_sys
	# Re-push possible 7th and 8th arg
	movq	24(%rsp), %r11
	pushq	%r11
	movq	24(%rsp), %r11
	pushq	%r11
	call	*%r10
	addq	$16, %rsp
.std_ret:
Lexit_hook:
	XTRACE_RESERVED_SPACE
	ret
.no_sys:
	movq	%rax, %rdi
	call	___unknown_syscall
	jmp		.std_ret

.section        __DATA,__data
.globl __darling_bsd_syscall_entry
.globl __darling_bsd_syscall_exit
__darling_bsd_syscall_entry:
	.quad Lentry_hook
__darling_bsd_syscall_exit:
	.quad Lexit_hook

#elif defined(__i386__)

#define copy_arg(off) \
	movl	8+off(%esp), %ecx ;\
	movl	%ecx, -56+off(%esp)

__darling_bsd_syscall:
Lentry_hook:
	XTRACE_RESERVED_SPACE
	calll	1f
1:
	popl	%ecx
2:
	movl	L___bsd_syscall_table$non_lazy_ptr-1b(%ecx), %ecx
	andl	$0xffff, %eax // Because there is some extra stuff in upper bytes we don't need
	movl	(%ecx,%eax,4), %eax
	test	%eax, %eax
	jz		.no_sys
	// Copy arguments
	copy_arg(0)
	copy_arg(4)
	copy_arg(8)
	copy_arg(12)
	copy_arg(16)
	copy_arg(20)
	copy_arg(24)
	copy_arg(28)
	copy_arg(32)
	copy_arg(36)
	copy_arg(40)
	copy_arg(44)
	copy_arg(48)
	copy_arg(52)
	subl	$56, %esp
	call	*%eax
	addl	$56, %esp
.std_ret:
Lexit_hook:
	XTRACE_RESERVED_SPACE
	ret
.no_sys:
	pushl	%ecx
	call	___unknown_syscall
	addl	$4, %esp
	jmp .std_ret

	.section        __IMPORT,__pointers,non_lazy_symbol_pointers
L___bsd_syscall_table$non_lazy_ptr:
	.indirect_symbol        ___bsd_syscall_table
	.long   0

.section        __DATA,__data
.globl __darling_bsd_syscall_entry
.globl __darling_bsd_syscall_exit
__darling_bsd_syscall_entry:
	.long Lentry_hook
__darling_bsd_syscall_exit:
	.long Lexit_hook

.subsections_via_symbols

#elif defined(__arm64__)

// On ARM64, the syscall number lives in x16
.globl __darling_handle_svc
.balign 4
__darling_handle_svc:
	stp fp, lr, [sp, #-16]!
	mov fp, sp
	
	// if (trap_no == 0x80000000)
	mov w9, 0x80000000
	cmp w16, w9
	b.ne not_platform_syscall

	// TODO: Do we need to implement platform syscalls?
	// Throw an exception
	brk #0x1

not_platform_syscall:
	// if (trap_no < 0) {
	cmp x16, #0
	b.pl trap_no_is_positive_or_zero_branch

		// if (trap_no == -3)
		cmn w8, #3
		b.eq mach_absolute_time_trap_branch
		// else if (trap_no == -4)
		cmn	w8, #4
		b.eq mach_continuous_time_trap_branch
		// else
		b darling_mach_syscall_branch

	mach_absolute_time_trap_branch:
		// Call mach_approximate_time() and return
		bl _mach_approximate_time
		b darling_svc_return
		
	mach_continuous_time_trap_branch:
		// Call mach_continuous_time() and return
		bl _mach_continuous_time
		b darling_svc_return
	
	darling_mach_syscall_branch:
		// Call mach syscalls and return 
		bl __darling_mach_syscall
		b darling_svc_return

trap_no_is_positive_or_zero_branch:
	// Call UNIX syscalls
	bl __darling_bsd_syscall
darling_svc_return:
	ldp fp, lr, [sp], #16
	ret

__darling_bsd_syscall:
	stp fp, lr, [sp, #-16]!
	mov fp, sp

Lentry_hook:
	XTRACE_RESERVED_SPACE

	// void* x9 = &__bsd_syscall_table 
	adrp	x9, ___bsd_syscall_table@PAGE
	add		x9, x9, ___bsd_syscall_table@PAGEOFF
	
	// void* x10 = __bsd_syscall_table[x16]
	mov		x10, x16
	lsl		x10, x10, #3 // x10 = x9 * 8 (size of a pointer)
	add		x10, x9, x10
	
	// x10(...)
	blr		x10
	
Lexit_hook:
	XTRACE_RESERVED_SPACE

	ldp fp, lr, [sp], #16
	ret

.section        __DATA,__data
.globl __darling_bsd_syscall_entry
.globl __darling_bsd_syscall_exit
__darling_bsd_syscall_entry:
	.quad Lentry_hook
__darling_bsd_syscall_exit:
	.quad Lexit_hook

#else
#	error Missing assembly
#endif

