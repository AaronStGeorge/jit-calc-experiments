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
- [ ] create a webassembly runtime using MLIR good enough to run fibonacci
	- [x] write a webassembly interpreter for practice (also only that's good enough to do fibonacci).
	- [ ] create an MLIR webassembly dialect.
	- [ ] write a compiler?,assembler? that takes serialized wasm and generates MLIR webasembly dialect.
	- [ ] write an MLIR pass on the webassembly dialect to insert instruction counts and swap calls when limit is reached.
	- [ ] lower webassembly dialect to LLVM


## What

Same as [`slow-roll-fibonacci-1`](../slow-roll-fibonacci-1).
