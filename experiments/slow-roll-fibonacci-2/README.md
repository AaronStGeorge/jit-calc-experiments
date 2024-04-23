# slow-roll-fibonacci-2

## Why

`slow-roll-fibonacci-2` solves the same problem as
[`slow-roll-fibonacci-1`](../slow-roll-fibonacci-1) but, in consideration to the
problems discussed in [`count-to-n`](../count-to-n), it uses a compiler to generate
calls to cooperative multitasking library rather than using pre-emptive
multitasking.

## How

### steps

- [x] get an MLIR fibonacci to run through LLVM
- [x] swap in and out of fibonacci at least once using library developed in
			count-to-n maybe just swapping out every instruction
- [x] take a look at the intermediate results get some idea to design the VM
	- "the VM" here is basically a (or maybe a set of) MLIR dialects where each
		MLIR operation is considered an operation on the computers game code runs on.
		So, we can say if each computer gets 400 cycles that equates to 400 operations
		if each operation is equally costly.
	- this could be existing MLIR dialects or a new one that maps fairly
		straight-forwardly to LLVM
	- (after a week + of thinking) I'm dumb! Why wouldn't this just be wasm?  This
		type of thing is like exactly what it was designed for. I'll take a look at
		some existing wasm runtimes, might not even need to roll our own here. Though
		TBH rolling our own sounds fun.
    - [x] find ex-post-facto excuse why rolling our own MLIR based wasm
      runtime is a necessary, high ROI, activity.
		- We'll likely want to run many copies of the same program, vectorizing this
			execution could lead to significant savings. We'll essentially have
			something like:
				for (int i = 0; i < num_copies; i++) {
					run_program(p, memories[i], inputs[i]);
				}
			We can run that loop in parallel, *or* do outer loop vectorization and
			shift the parallelism (at least somewhat) into the instructions. That
			could really speed things up!
		- We'll need somewhere to insert the swap calls, an MLIR pass would be a
			very nice way to do this.
- [ ] create a webassembly runtime using MLIR good enough to run fibonacci with swap calls
	- [x] write a webassembly interpreter for practice (also only that's good enough to do fibonacci).
	- [ ] ~~create an MLIR webassembly dialect~~.
		- a wasm dialect probably isn't necessary. We'll definitely need a "towervm"
		dialect (or something) with instructions that indicate the places we should
		increment instruction count, and places to check to see if it's time to swap
		out. We *could* create a wasm dialect then insert the "towervm" instructions
		between the wasm instructions before lowering the wasm dialect to "standard"
		MLIR. *Or*, we could just generate the MLIR that the wasm dialect would
		lower to + generate the "towervm" instructions between each "chunk" of
		lowered MLIR. I think the latter is a little easier and relatively risk
		free. We'll need to lower + insert "towervm" instructions in one step as we
		won't have any way to back out the wasm provenance of MLIR instructions
		after lowering. I don't love welding those steps together, but it shouldn't
		be the hardest thing to break apart later should things get ugly.
	- [x] write a compiler?,assembler? that takes serialized wasm and generates
	MLIR that can be compiled down to machine code and executed
		- [x] for add.c
		- [x] for fib.c
			- [x] for simple_cond.c (simpler branching behavior + CFG)
				- [x] expand decoder to include import section so simple_cond.wasm can be decoded
                  - [x] import section decode
                  - [x] instruction decode
				- [x] expand interpreter to include instructions needed for simple_cond.wasm
                  - It's probably easiest to just create fake functions for the
                  calls to imported functions in simple_cond.wasm
	- [ ] Insert instruction counts and swap calls when limit is reached.
	- [ ] ~~lower webassembly dialect to LLVM~~


## What

Same as [`slow-roll-fibonacci-1`](../slow-roll-fibonacci-1).
