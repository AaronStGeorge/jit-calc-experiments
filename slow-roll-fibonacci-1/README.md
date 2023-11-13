# slow-roll-fibonacci-1

## Why

tl;dr To have a game that runs player functions but doesn't freeze we need a
way to break up a long running functions into controllable bite sized pieces.

Imagine we're in a game that's needs to produce a frame every `x` milliseconds.
This game should compute a sate of the world at every "tick" `x` milliseconds
apart: `t1`, `t2`, `t3`, etc.  In such a game, we could have a function
"`game_function`" that computes a result `r1` based on `t1` data.  This function
should always run in < `x` milliseconds.

Here's a sequence diagram---with the vertical length a segment indicating how
long a function is running---for some imaginary "`game_function_1`":
```
-----------------------------------------------------------------------------
   caller        game_function_1
   -----------------------------
   ↓
   |
   > t3` data   >   // caller calls game_function_1 with t3 data
                 |   // game_function_1 does calculation c1 based on t3 data
   < `r3` result <   // game_function_1 returns result
   |                 // caller does stuff with result
   ↓
-----------------------------------------------------------------------------
```

For a longer running function "`game_function_2`" the sequence diagram would
look like this:
```
-----------------------------------------------------------------------------
   caller        game_function_2
   -----------------------------
   ↓
   |
   > `t3` data   >   // caller calls game_function_2 with t3 data
                 |   // game_function_2 does calculation c1 based on t3 data
                 |   // game_function_2 does calculation c2 based on t3 data
   < `r3` result <   // game_function_2 returns result
   |                 // caller does stuff with result
   ↓
-----------------------------------------------------------------------------
```

Now imagine this game needs to run player created functions with unknown running
time (very possibly > `x` milliseconds). If we just call "`player_function`" the
sequence diagram could look like this:
```
-----------------------------------------------------------------------------
   caller        player_function
   -----------------------------
   ↓
   |
   > `t3` data   >   // caller calls player_function with t3 data
                 |   // player_function does calculation c1 based on t3 data
                 |   // player_function does calculation c2 based on t3 data
                 |   // player_function does calculation c3 based on t3 data
                 |   // player_function does calculation c4 based on t3 data
   < `r3` result <   // player_function returns result
   |                 // caller does stuff with result
   ↓
-----------------------------------------------------------------------------
```
If we imagine that for however much latency one vertical line represents, two is
acceptable in our game, then the above sequence diagram represents a problem.
`player_function` took four lines. We need some way to handle long running
functions like this.

## How (strategy)

A potential solution is to cap a player functions running time for any
individual tick, but allow the function to run over multiple ticks. If a player
function runs over time, the game will use a result computed on a previous tick
(or a default if no results have been calculated yet).

We want to context switch in and out of our player function to ensure that the
game can do what it needs to do in a tick, while also ensuring the player
function makes progress.  We don't want to restart a players function each
tick. If we did that, the function would always hit the latency barrier and
never complete. Instead, we want to "spread" the function over multiple ticks.
Similar to how an OS keeps multiple processes making progress on a single thread
by context switching between them.

Here's a sequence diagram with the above player_function spread over multiple
ticks:
```
-----------------------------------------------------------------------------
   caller        player_function
   -----------------------------
   ↓
   |
   > `t3` data   >   // caller calls player_function with t3 data
                 |   // player_function does calculation c1 based on t3 data
                 |   // player_function does calculation c2 based on t3 data
                 ~   // player_function is interrupted
   < `r2` result <   // previously computed result is returned
   |                 // caller does stuff with result
   ↓                 //  ...
   |                 // next tick
   > `t4` data   >   // caller calls player_function with t4 data
                 ~   // previous player_function execution is restarted
                 |   // player_function does calculation c3 based on t3 data
                 |   // player_function does calculation c4 based on t3 data
   < `r3` result <   // player_function returns result based on t3 data
   |                 // caller does stuff with result
   ↓
   > `t5` data   >   // caller calls player_function with t5 data
                 |   // player_function does calculation c1 based on t5 data
                 ↓
-----------------------------------------------------------------------------
```
A system like this could run arbitrary running time player functions while
keeping the game moving.

#### Notes
If we imagine that for however much latency one vertical line represents, each
player function is allowed a "window" of four to run:
```
...  // game code running
---
     // player code allowed to run
     // player code allowed to run
     // player code allowed to run
     // player code allowed to run
---
...  // game code running
```
If a players function runs over a tick boundary but completes on the next,
there's an option of packing the start of the the next player function into the
current window. The new invocation would run with whatever time is left in that
window. Example:"

```
...  //game code running
--
   > // player_function called with t1 data
   | // computing
   | // computing
   ~ // interrupted
--
...  //game code running
--
    ~ // restarted
    | // computing
    < // returns based on t1 data
    > // we *could* start player_function again with t2 data here
--
...
```

I'd vote to keep it simple and say that only a single invocation is ever running
in any given window. So, in the example above the player function would never
get `t2` data, it would only be called with `t3` data.

## How (tactics)

`libc` provides [makecontext, swapcontext, and
getcontext](https://man7.org/linux/man-pages/man3/makecontext.3.html) to capture
the state of a machine's registers. That *is* the sate of the machine, if you
change those you change the command stream a machine is executing. The captured
state can be stored, restarted, and manipulated. Let's use that!

[This](https://c9x.me/articles/gthreads/intro.html) blog post has a pretty good
introduction to the general idea. Though, it only goes over a user-land
threading library that switches at function boundaries, we will switch context
at any point during execution.

## What

A race between computing fibonacci the fast way and slow way. The fast way will
be calculated by "the game" which cannot be interrupted, the slow way will be
computed by the interruptible "player function".  Every "tick" we run the fast
way to calculate the nth element of fibonacci, then put the slow way on our of
our interruptible threads which will stop in `x` milliseconds regardless of if
it has completed, then resume the next tick.

If everything is working we should see results produced every `x` milliseconds,
with the user code gradually falling behind while still computing the last
request that it saw. Output should look something like the following:
```
Fibonacci game!
game user
   0    0
   1    1
   1    1
   2    2
   3    3
   5    5
   8    8
  13   13
  21   21
  34   34
  55   55
  89   89
 144  144
 233  144
 377  233
 610  233
 987  233
1597  610
2584  610
4181  610
```
