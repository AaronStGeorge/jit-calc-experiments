.global _swap
.global _start
.global _stop
.align 4

.macro SWAP
	MOV x9,  sp
	STR x9,  [x0]
	STR lr,  [x0, #8]
	STR fp,  [x0, #16]
	STR x19, [x0, #24]
	STR x20, [x0, #32]
	STR x21, [x0, #40]
	STR x22, [x0, #48]
	STR x23, [x0, #56]
	STR x24, [x0, #64]
	STR x25, [x0, #72]
	STR x26, [x0, #80]
	STR x27, [x0, #88]
	STR x28, [x0, #96]

	LDR x9,  [x1]
	MOV sp,  x9
	LDR lr,  [x1, #8]
	LDR fp,  [x1, #16]
	LDR x19, [x1, #24]
	LDR x20, [x1, #32]
	LDR x21, [x1, #40]
	LDR x22, [x1, #48]
	LDR x23, [x1, #56]
	LDR x24, [x1, #64]
	LDR x25, [x1, #72]
	LDR x26, [x1, #80]
	LDR x27, [x1, #88]
	LDR x28, [x1, #96]
.endm

_swap:
	SWAP
	MOV x0, 0
	RET

_start:
	SWAP
	RET

_stop:
	SWAP
	MOV x0, 1
	RET
