# count-to-n

## Why

Much is right in the way [`slow-roll-fibonacci-1`](../slow-roll-fibonacci-1)
does things. Each player program has a known stack size, a player program's
stack isn't connected to the game code's stack, a player's program can be paused
and restarted, return values from a previous run of a player's program can be
used as the current return value, the memory used by the player's program(s) can
be inspected by the game code. Things are pretty great.

But, there's some issues too. The whole preemptive scheduling thing is fun, but
[unless we're super careful][summon] it will [summon the giant lake
troll][a_troll] of undefined behavior, which would not be fun. `swapcontext`ing
in and out of a player's programs willy-nilly requires a lot of care around
re-entrancy. Being careful is hard.

We could solve the same problems `slow-roll-fibonacci-1` was addressing with
cooperative multitasking. Player programs would swap back to game code with a
`yield` call and game code back into a players program with a `run` call... But
that would never work, we can't trust the players to insert `yield` calls into
their code.

Oh wait, we're the compiler! We can just generate `yield` calls in the right
spots!  Maybe after every "instruction" on the "machine" the player's code runs
on. Game code can decide after each `yield` whether to `run` that player program
more, or go do some game-y stuff, or run a different player program. Being the
compiler is cool!

We need a couple things:
1. a user land context switching library.
2. a compiler capable of generating calls to said context switching library at the appropriate points.

First things first, lets build the context switching library.

## How

There's lots of great options around already. The only constraints are, assuming
we're using an MLIR based compiler for 2, it'll be easiest to use a context
switching library that uses that C System-V ABI for calls into functions. So one
for golang, for example, won't work. ChatGpt recommends:

- libco: This is a lightweight, performance-focused library for coroutines.

- libtask: Created by Russ Cox, this C library provides task (coroutine) creation and management features.

But it also suggests:

- Roll your own with assembly: For the utmost in control and performance (but at the cost of portability and complexity), you can implement context switching directly in assembly. Also, I triple dog dare you.

Who are we to argue with our robot overlords?

## What

A cooperative context switching library in assembly, and a really simple test
program. The test program just counts whatever `n` is passed to it yielding
control after each increment, then returns `n`.

<!-- (jklp) ../(run
) -->

[summon]: https://github.com/AaronStGeorge/jit-calc-experiments/blob/c845524ac15d89bae61a82d3436cd4b294e663c1/slow-roll-fibonacci/slow-roll-fibonacci.c#L98-L107
[a_troll]: https://www.youtube.com/watch?v=4_SNmG1p_Bg