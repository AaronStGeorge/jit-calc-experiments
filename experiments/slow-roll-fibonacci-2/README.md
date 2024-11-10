# slow-roll-fibonacci-2

## Running

You'll need everything built, and you'll need to source [`source_me_to_setup_slow-roll-fibonacci-2_experiment.sh`](https://github.com/AaronStGeorge/jit-calc-experiments/blob/eb01f6996c6c13cb7d303c2893d5192308766cc6/source_me_to_setup_slow-roll-fibonacci-2_experiment.sh).

```shell
$ python python/wasm_to_mlir.py test/experiments/slow-roll-fibonacci-2/fib.wasm \
	| jit-calc-opt --shim-function='function-to-shim=fib' --convert-tower-runtime-to-llvm \
	| srf2-driver
```

## Why

`slow-roll-fibonacci-2` solves the same problem as
[`slow-roll-fibonacci-1`](../slow-roll-fibonacci-1) but, in consideration to the
problems discussed in [`count-to-n`](../count-to-n).

## How

The strategy hasn't changed from [`slow-roll-fibonacci-1`](../slow-roll-fibonacci-1#how-(strategy)), but the [tactics](../slow-roll-fibonacci-1#how-(tatctics)) have.
`slow-roll-fibonacci-2` uses a custom compiler to generate calls to a cooperative multitasking library.
We no longer swap in and out of user programs at random points when a timer pings.
`slow-roll-fibonacci-2`'s custom compiler interleaves "runtime control" instructions [`increment_count`](https://github.com/AaronStGeorge/jit-calc-experiments/blob/b9579f6c6e32408bff9ff280a0f54e1ac9741025/include/jit-calc-mlir/Dialect/TowerRuntime/IR/TowerRuntimeOps.td#L38-L50) and [`check_swap`](https://github.com/AaronStGeorge/jit-calc-experiments/blob/b9579f6c6e32408bff9ff280a0f54e1ac9741025/include/jit-calc-mlir/Dialect/TowerRuntime/IR/TowerRuntimeOps.td#L22-L36) around user instructions.
[`increment_count`](https://github.com/AaronStGeorge/jit-calc-experiments/blob/b9579f6c6e32408bff9ff280a0f54e1ac9741025/include/jit-calc-mlir/Dialect/TowerRuntime/IR/TowerRuntimeOps.td#L38-L50) counts user-instructions, and [`check_swap`](https://github.com/AaronStGeorge/jit-calc-experiments/blob/b9579f6c6e32408bff9ff280a0f54e1ac9741025/include/jit-calc-mlir/Dialect/TowerRuntime/IR/TowerRuntimeOps.td#L22-L36) swaps out of user code when user-instruction count reaches a limit set by game code.

To make that a little more concrete let's walk through an example, take the following C code
```c
int a_func();
int b_func();

int cond(int n) {
    if (n > 5) {
        return a_func();
    }
    return b_func();
}
```

`slow-roll-fibonacci-2` wants to run a set number of "instructions" from the above program, then swap back into user code using a [swap mechanism](https://c9x.me/articles/gthreads/intro.html).
The first thing `slow-roll-fibonacci-2` needs is a rigorous definition of what a user code "instruction" actually is.

We chose `wasm` as the instruction set. Here's the pitch for wasm:
- `wasm` is well defined and relatively small.
- `wasm` was designed for security, there likely could be some security benefits down the line.
-  A big pile of compilers already exist that will hand us `wasm` instructions for a user program in various languages.

Hading the above C program to clang and asking for `wasm` back we get the program below
```wasm
 (func $cond (type 2) (param i32) (result i32)
    block  ;; label = @1
      local.get 0
      i32.const 6
      i32.lt_s
      br_if 0 (;@1;)
      call $a_func
      return
    end
    call $b_func)
```
We then convert that to an MLIR based representation, and interleave the [`check_swap`](https://github.com/AaronStGeorge/jit-calc-experiments/blob/b9579f6c6e32408bff9ff280a0f54e1ac9741025/include/jit-calc-mlir/Dialect/TowerRuntime/IR/TowerRuntimeOps.td#L22-L36) and [`increment_count`](https://github.com/AaronStGeorge/jit-calc-experiments/blob/b9579f6c6e32408bff9ff280a0f54e1ac9741025/include/jit-calc-mlir/Dialect/TowerRuntime/IR/TowerRuntimeOps.td#L38-L50) instructions using our [custom compiler](https://github.com/AaronStGeorge/jit-calc-experiments/blob/eb01f69/experiments/slow-roll-fibonacci-2/python/wasm_to_mlir.py).
```mlir
module {
  func.func private @a_func() -> i32
  func.func private @b_func() -> i32
  func.func @cond(%arg0: !tower_runtime.ctx, %arg1: i32) -> i32 {
    %c0 = arith.constant 0 : index
    %alloca = memref.alloca() : memref<1xi32>
    memref.store %arg1, %alloca[%c0] : memref<1xi32>
    cf.br ^bb1(%arg0 : !tower_runtime.ctx)
  ^bb1(%0: !tower_runtime.ctx):  // pred: ^bb0
    %1 = tower_runtime.check_swap %0 : !tower_runtime.ctx
    %2 = tower_runtime.increment_count %1 : !tower_runtime.ctx
    %3 = memref.load %alloca[%c0] : memref<1xi32>
    %4 = tower_runtime.check_swap %2 : !tower_runtime.ctx
    %5 = tower_runtime.increment_count %4 : !tower_runtime.ctx
    %c6_i32 = arith.constant 6 : i32
    %6 = tower_runtime.check_swap %5 : !tower_runtime.ctx
    %7 = tower_runtime.increment_count %6 : !tower_runtime.ctx
    %8 = arith.cmpi slt, %3, %c6_i32 : i32
    %9 = tower_runtime.check_swap %7 : !tower_runtime.ctx
    %10 = tower_runtime.increment_count %9 : !tower_runtime.ctx
    cf.cond_br %8, ^bb3(%10 : !tower_runtime.ctx), ^bb2(%10 : !tower_runtime.ctx)
  ^bb2(%11: !tower_runtime.ctx):  // pred: ^bb1
    %12 = tower_runtime.check_swap %11 : !tower_runtime.ctx
    %13 = call @a_func() : () -> i32
    return %13 : i32
  ^bb3(%14: !tower_runtime.ctx):  // pred: ^bb1
    %15 = tower_runtime.check_swap %14 : !tower_runtime.ctx
    %16 = call @b_func() : () -> i32
    return %16 : i32
  }
}
```
This representation needs a few custom lowering passes ([`--shim-function`](https://github.com/AaronStGeorge/jit-calc-experiments/blob/eb01f6996c6c13cb7d303c2893d5192308766cc6/include/jit-calc-mlir/Dialect/TowerRuntime/Transforms/Passes.td#L6-L17), [`--convert-tower-runtime-to-llvm`](https://github.com/AaronStGeorge/jit-calc-experiments/blob/eb01f6996c6c13cb7d303c2893d5192308766cc6/include/jit-calc-mlir/Conversion/Passes.td#L18-L27)), but otherwise some slapped together [LLVM mumbo jumbo](https://github.com/AaronStGeorge/jit-calc-experiments/blob/eb01f6996c6c13cb7d303c2893d5192308766cc6/experiments/slow-roll-fibonacci-2/srf2-driver/srf2-driver.cpp) can lower the above representation down to executable code. The result turns out to work very similar to how [`jit-calc`](../jit-calc) worked.

## What

Same as [`slow-roll-fibonacci-1`](../slow-roll-fibonacci-1).
