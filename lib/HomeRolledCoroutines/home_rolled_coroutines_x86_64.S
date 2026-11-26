// SysV x86_64 context switching for HomeRolledCoroutines on Linux.
// Register offsets follow struct context in CoroutineContext.h.

#if !defined(__linux__) || !defined(__x86_64__) || defined(__ILP32__)
#error Expected a Linux x86_64 compiler target
#endif

.text
.globl swap, start, stop
.p2align 4

// Save the caller's stack pointer and the return address separately.
.macro SAVE ctx
	leaq 8(%rsp), %r11
	movq %r11, 0(\ctx)
	movq (%rsp), %r11
	movq %r11, 8(\ctx)
	movq %rbp, 16(\ctx)
	movq %rbx, 24(\ctx)
	movq %r12, 32(\ctx)
	movq %r13, 40(\ctx)
	movq %r14, 48(\ctx)
	movq %r15, 56(\ctx)
	stmxcsr 64(\ctx)
	fnstcw 68(\ctx)
.endm

// Restore the stack pointer and load the continuation into r11.
.macro RESTORE ctx, fresh=0
	movq 0(\ctx), %rsp
	movq 8(\ctx), %r11
	movq 16(\ctx), %rbp
	movq 24(\ctx), %rbx
	movq 32(\ctx), %r12
	movq 40(\ctx), %r13
	movq 48(\ctx), %r14
	movq 56(\ctx), %r15
	.if !\fresh
	ldmxcsr 64(\ctx)
	fldcw 68(\ctx)
	.endif
.endm

swap:
	SAVE %rdi
	RESTORE %rsi
	xorl %eax, %eax
	pushq %r11
	retq

start:
	// Pass rdi/rsi/rdx/rcx unchanged, and inherit the caller's FP environment.
	SAVE %rdi
	RESTORE %rsi, 1
	// The fresh stack is 16-byte aligned before call, as required by SysV.
	callq *%r11
	ud2 // Entry functions must transfer control back with stop.

stop:
	SAVE %rdi
	RESTORE %rsi
	movl $1, %eax
	pushq %r11
	retq

.section .note.GNU-stack,"",@progbits
