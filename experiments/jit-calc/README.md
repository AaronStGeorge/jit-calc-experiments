# jit-calc

We want a basic command line calculator... but wouldn't it be fun if it were
massively over-engineered? We could implement a simple interpreter to do the
parse-y and calculation-y bits of a calculator in maybe eighty lines of C++.
But eighty lines of C++ isn't cool, you know what's cool? Eight million lines
of C++. So let's use LLVM!

By using LLVM, we can create an entirely different type of calculator. To do
a calculation, a normal calculator executes instructions from its binary. If
we smuggle a compiler inside the calculator binary, it can *generate* the
instructions required to do a calculation. The calculator binary won't need
to physically contain the instructions necessary to do the calculation, it
can build them on the fly!

This is hot stuff! Using LLVM, we can cut the calculator's binary size down by
*dozens* of bytes, and we'll only need to pack a few gigs of compiler code in
there to do so! When the user requests some calculation be done, we can compile
their request in *milliseconds*. A computer requires a single digit number of
nanoseconds to execute a pre-compiled arithmetic instruction. Our users won't
have to slog through that! This is an excellent trade.

----

Next Steps:

- [x] Learn enough about ORC APIs to materialize a static MLIR function with C ABI wrappers to add two numbers as a callable function.
- [ ] Abandon all pretext that this is a calculator. Admit that it's a compiler + runtime system similar to a javascript engine.
- [ ] Expand static MLIR blobs. Based on the given user command, the blobs should execute a more interesting operations than add, on more interesting runtime objects than intergers.
- [ ] Rather than relying on a static pre-written pile of MLIR, actually generate the MLIR based on command from user.
- [ ] Extend into golang compiler that enables player programed NPCs in a game.
