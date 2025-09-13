# `hover-game`

## Why

Wouldn't it be cool to see the custom compiler developed in
[`slow-roll-fibonacci-2`](../slow-roll-fibonacci-2) actually do something?
Doing stuff is hard though... So what if it was something... stupid?

## How

Rather than using ORC + LLVM like we did in [`jit-calc`](../jit-calc) we're
going to use some [CMake fuckery](../../cmake/JitCalcUtils.cmake) to link the
generated code into the executable. We can run a function that does some cool
stuff---or some dumb stuff--through the compiler, slap some raylib around and
boom, Game!

## What

A "game" that allows you to hover over a circle, the color of the circle on
hover is decided by code run through the custom compiler!