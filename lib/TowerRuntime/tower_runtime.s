.global _start
.global _stop
.align 4

_start:
	// Load pointer to main context. We're swapping from this context, so we need
	// to populate the struct with current values of registers.
	ldr x9,  [x0]
	// Load pointer to thread context. We're swapping to this context, so the
	// existing values should be garbage. Whatever called start is responsible for
	// ensuring that it's populated.
	ldr x10, [x0, #8]

	// You can't store the stack pointer to memory address, but you can store it
	// to a register then store the resister to a memory address.
	MOV x11, sp

	// Dump machine state into context we're swapping from.
	STR x11, [x9]
	STR lr,  [x9, #8]
	STR fp,  [x9, #16]
	STR x19, [x9, #24]
	STR x20, [x9, #32]
	STR x21, [x9, #40]
	STR x22, [x9, #48]
	STR x23, [x9, #56]
	STR x24, [x9, #64]
	STR x25, [x9, #72]
	STR x26, [x9, #80]
	STR x27, [x9, #88]
	STR x28, [x9, #96]

	// Load machine state from context we're swapping to.
	LDR x11, [x10]
	MOV sp,  x11
	LDR lr,  [x10, #8]
	LDR fp,  [x10, #16]
	LDR x19, [x10, #24]
	LDR x20, [x10, #32]
	LDR x21, [x10, #40]
	LDR x22, [x10, #48]
	LDR x23, [x10, #56]
	LDR x24, [x10, #64]
	LDR x25, [x10, #72]
	LDR x26, [x10, #80]
	LDR x27, [x10, #88]
	LDR x28, [x10, #96]
	RET

_stop:
	// Load pointer to main context. It had better be populated!
	ldr x10, [x0]

	// Load machine state from main context.
	LDR x11, [x10]
	MOV sp,  x11
	LDR lr,  [x10, #8]
	LDR fp,  [x10, #16]
	LDR x19, [x10, #24]
	LDR x20, [x10, #32]
	LDR x21, [x10, #40]
	LDR x22, [x10, #48]
	LDR x23, [x10, #56]
	LDR x24, [x10, #64]
	LDR x25, [x10, #72]
	LDR x26, [x10, #80]
	LDR x27, [x10, #88]
	LDR x28, [x10, #96]
	RET