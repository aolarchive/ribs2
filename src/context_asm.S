.globl ribs_swapcontext
ribs_swapcontext:
	/* Save the preserved registers and the return address. */
	movq	%rbx, 0(%rsi)
	movq	%rbp, 8(%rsi)
	movq	%r12, 16(%rsi)
	movq	%r13, 24(%rsi)
	movq	%r14, 32(%rsi)
	movq	%r15, 40(%rsi)

	movq	(%rsp), %rcx
	movq	%rcx, 48(%rsi)
	leaq	8(%rsp), %rcx		/* Exclude the return address.  */
	movq	%rcx, 56(%rsi)

ribs_setcontext:
	/* Load the new stack pointer and the preserved registers.  */
	movq	0(%rdi), %rbx
	movq	8(%rdi), %rbp
	movq	16(%rdi), %r12
	movq	24(%rdi), %r13
	movq	32(%rdi), %r14
	movq	40(%rdi), %r15
	movq	48(%rdi), %rcx
        movq	56(%rdi), %rsp

	pushq	%rcx

	ret

.globl __ribs_context_exit
__ribs_context_exit:
        movq %rbx, %rdi
        jmp ribs_setcontext


