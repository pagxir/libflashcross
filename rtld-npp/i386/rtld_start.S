	.text
	.align	4
	.globl	_rtld_fixup_start
	.type	_rtld_fixup_start,@function
_rtld_fixup_start:
	pushf				# Save eflags
	pushl	%eax			# Save %eax
	pushl	%edx			# Save %edx
	pushl	%ecx			# Save %ecx
	pushl	20(%esp)		# Copy reloff argument
	pushl	20(%esp)		# Copy obj argument

	call	_rtld_fixup@PLT		# Transfer control to the binder
	/* Now %eax contains the entry point of the function being called. */

	addl	$8,%esp			# Discard binder arguments
	movl	%eax,20(%esp)		# Store target over obj argument
	popl	%ecx			# Restore %ecx
	popl	%edx			# Restore %edx
	popl	%eax			# Restore %eax
	popf				# Restore eflags
	leal	4(%esp),%esp		# Discard reloff, do not change eflags
	ret				# "Return" to target address
