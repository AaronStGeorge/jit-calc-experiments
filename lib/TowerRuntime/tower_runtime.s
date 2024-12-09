.global _start
.global _stop
.global _swap
.align 4

// Read machine state into context at pointer p_ctx. READ will use register
// sp_reg to transfer stack pointer; ARM doesn't allow sp register to be read
// into a memory address, but you can be read into register that's then read
// into a memory address.
.macro READ p_ctx, sp_reg
	MOV \sp_reg, sp
	STR \sp_reg, [\p_ctx]
	STR lr,  [\p_ctx, #8]
	STR fp,  [\p_ctx, #16]
	STR x19, [\p_ctx, #24]
	STR x20, [\p_ctx, #32]
	STR x21, [\p_ctx, #40]
	STR x22, [\p_ctx, #48]
	STR x23, [\p_ctx, #56]
	STR x24, [\p_ctx, #64]
	STR x25, [\p_ctx, #72]
	STR x26, [\p_ctx, #80]
	STR x27, [\p_ctx, #88]
	STR x28, [\p_ctx, #96]
.endm

// Write context from pointer p_ctx into machine state. WRITE will use register
// sp_reg to transfer stack pointer; ARM doesn't allow sp register to be set
// from a memory address, but you can set it from a register that's set from a
// memory address.
.macro WRITE p_ctx, sp_reg
	LDR \sp_reg, [\p_ctx]
	MOV sp,  \sp_reg
	LDR lr,  [\p_ctx, #8]
	LDR fp,  [\p_ctx, #16]
	LDR x19, [\p_ctx, #24]
	LDR x20, [\p_ctx, #32]
	LDR x21, [\p_ctx, #40]
	LDR x22, [\p_ctx, #48]
	LDR x23, [\p_ctx, #56]
	LDR x24, [\p_ctx, #64]
	LDR x25, [\p_ctx, #72]
	LDR x26, [\p_ctx, #80]
	LDR x27, [\p_ctx, #88]
	LDR x28, [\p_ctx, #96]
.endm


.macro SWAP_OUT
.endm

// Swap from main into thread.
_start:
	// Load pointer to main context. We're swapping from this context, so we need
	// to populate the struct with current values of registers.
	ldr x9,  [x0]
	// Load pointer to thread context. We're swapping to this context. Whatever
	// called into this is responsible for ensuring that the context is populated
	// with reasonable stuff.
	ldr x10, [x0, #8]

	// Dump  current machine state into main context.
	READ x9, x11

	// Load machine state from thread context.
	WRITE x10, x11
	RET

_swap:
	// Dump  current machine state into "from" argument
	READ x0, x9

	// Load machine state from "to" argument
	WRITE x1, x9

	// return false
	mov x0, #0
	RET

// Swap from thread into main.
_stop:
	// Load pointer to main context. We're swapping to this context.
	ldr x9,  [x0]

	// Load machine state from main context.
	WRITE x9, x11
	// return true
	mov x0, #1
	RET