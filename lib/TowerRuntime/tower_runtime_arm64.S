// ARM64 context switching for TowerRuntime on macOS and Linux.
// Offsets follow MachineState and TowerRuntimeContext in TowerRuntime.h.

#if !defined(__aarch64__) && !defined(__arm64__)
#error Expected an ARM64 compiler target
#endif

#ifdef __APPLE__
#define CNAME(name) _##name
#else
#define CNAME(name) name
#endif

.text
.globl CNAME(swap), CNAME(start), CNAME(stop)
.p2align 2

// Save machine state into the context at ctx. AArch64 cannot store sp
// directly, so use x11 to transfer it to memory.
.macro SAVE ctx
	MOV x11, sp
	STR x11, [\ctx]
	STR lr,  [\ctx, #8]
	STR fp,  [\ctx, #16]
	STP x19, x20, [\ctx, #24]
	STP x21, x22, [\ctx, #40]
	STP x23, x24, [\ctx, #56]
	STP x25, x26, [\ctx, #72]
	STP x27, x28, [\ctx, #88]
	STP d8,  d9,  [\ctx, #104]
	STP d10, d11, [\ctx, #120]
	STP d12, d13, [\ctx, #136]
	STP d14, d15, [\ctx, #152]
	MRS x11, fpcr
	STR x11, [\ctx, #168]
.endm

// Restore machine state from ctx. Load the saved stack pointer into x11
// first, since AArch64 cannot load it directly into sp.
.macro RESTORE ctx, fresh=0
	LDR x11, [\ctx]
	MOV sp, x11
	LDR lr,  [\ctx, #8]
	LDR fp,  [\ctx, #16]
	LDP x19, x20, [\ctx, #24]
	LDP x21, x22, [\ctx, #40]
	LDP x23, x24, [\ctx, #56]
	LDP x25, x26, [\ctx, #72]
	LDP x27, x28, [\ctx, #88]
	.if !\fresh
	LDP d8,  d9,  [\ctx, #104]
	LDP d10, d11, [\ctx, #120]
	LDP d12, d13, [\ctx, #136]
	LDP d14, d15, [\ctx, #152]
	LDR x11, [\ctx, #168]
	MSR fpcr, x11
	.endif
.endm

// Save the "from" context and resume the "to" context, returning false
// to the resumed context to report a yield.
CNAME(swap):
	SAVE x0
	RESTORE x1
	MOV x0, 0
	RET

// Switch from main into a fresh coroutine whose stack and entry point
// have already been initialized by the caller.
CNAME(start):
	// Load the main (from) and thread (to) contexts from TowerRuntimeContext.
	// Pass x0-x3 unchanged, and inherit the caller's FP environment.
	LDP x9, x10, [x0]
	SAVE x9
	RESTORE x10, 1
	MOV x9, lr
	BLR x9
	BRK #0 // Entry functions must transfer control back with stop.

// Switch back to main without saving the finished coroutine. Return true
// to the resumed main context to report completion.
CNAME(stop):
	LDR x9, [x0]
	RESTORE x9
	MOV x0, 1
	RET

#ifdef __ELF__
.section .note.GNU-stack,"",%progbits
#endif
