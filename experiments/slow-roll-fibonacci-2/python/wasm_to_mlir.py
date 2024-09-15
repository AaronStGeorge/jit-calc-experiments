#!/usr/bin/env python3
import argparse
from abc import ABC
from dataclasses import dataclass, field
from typing import List, Optional, Dict, Callable, Any

import wasm
import jitcalc.ir as mlir
import jitcalc.extras.types as T
from jitcalc.dialects import (
    arith,
    cf,
    func,
    memref,
    tower_runtime,
    wasm as wasm_d,
)


def is_terminator(instruction: wasm.Instruction) -> bool:
    terminators = [wasm.br, wasm.wasm_return]
    return any([isinstance(instruction, terminator) for terminator in terminators])


@dataclass
class Function(ABC):
    name: str
    inputs: List[mlir.Type]
    results: List[mlir.Type]


@dataclass
class ImportedFunction(Function):
    mlir_func: func.FuncOp


@dataclass
class DefinedFunction(Function):
    code: wasm.Code


@dataclass
class Translator(wasm.InstructionVisitor[None]):
    indices: List[Optional[mlir.Value]]
    block: (
        mlir.Block
    )  # current block we're dumping code into, we need a reference so we can call "add block after" on
    ip: mlir.InsertionPoint
    functions: List[
        Function
    ]  # wasm functions kept around so we can generate func.call ops
    index_ip: (
        mlir.InsertionPoint
    )  # insertion point at top of function (used for indices to keep them grouped)
    wasm_locals: List[mlir.Value]
    last_context: tower_runtime.ContextType
    stack: List[mlir.Value] = field(default_factory=list)
    jump_targets: List[mlir.Block] = field(default_factory=list)

    # HELPER FUNCTIONS =========================================================

    @staticmethod
    def wrap_with_increment_and_check_swap(function):
        """
        Decorator to wrap visitors for instructions without any control flow
        with tower_runtime.increment_count and tower_runtime.check_swap
        generation.

        Updates function that would have generated:
            %2 = arith.addi %0, %1 : i32
        to one that generates:
            %2 = tower_runtime.increment_count %arg0 : !tower_runtime.ctx
            %3 = arith.addi %0, %1 : i32
            tower_runtime.check_swap %2 : !tower_runtime.ctx
        """

        def wrapper(self: "Translator", *args, **kwargs):
            self.last_context = tower_runtime.increment_count(
                self.last_context, ip=self.ip
            )
            result = function(self, *args, **kwargs)
            tower_runtime.check_swap(self.last_context, ip=self.ip)
            return result

        return wrapper

    def move_ip_to_block(self, block: mlir.Block):
        """
        Updates generation insertion point + associated state.

        Blocks should always start with check_swap. A jump instruction into the
        block may have bumped the count over the limit.
        """
        self.block = block
        self.ip = mlir.InsertionPoint(block)
        self.last_context = block.arguments[0]
        tower_runtime.check_swap(self.last_context, ip=self.ip)

    def push(self, item: mlir.Value):
        """
        Push wasm op generation stack.

        Wasm op generation stack adapts wasm stack machine to mlir register
        machine at compile time.
        """
        self.stack.append(item)

    def pop(self):
        """
        Pop wasm op generation stack

        Wasm op generation stack adapts wasm stack machine to mlir register
        machine at compile time.
        """
        return self.stack.pop()

    def peek(self):
        """
        Peek wasm op generation stack

        Wasm op generation stack adapts wasm stack machine to mlir register
        machine at compile time.
        """
        return self.stack[-1]

    def binary(self, op_func: Callable[[mlir.Value, mlir.Value], mlir.Value]):
        """
        Create binary wasm op
        """
        rhs = self.pop()
        lhs = self.pop()
        result = op_func(lhs, rhs)
        self.push(result)

    def get_mlir_index(self, index: int) -> mlir.Value:
        """
        get_mlir_index ensures that we only create one `arith.constant` operation for each constant we need, and ensures
        that all constants are created in a group at the top of the function. For example, when pushing wasm locals
        `%arg0` and `%arg1` onto the stack (mem2reg will clean up the stack later) we only want to create one `%c0`
        value.
            func.func @add(%arg0: i32, %arg1: i32) -> i32 {
                %c0 = arith.constant 0 : index
                %alloca = memref.alloca() : memref<1xi32>
                memref.store %arg0, %alloca[%c0] : memref<1xi32>
                %alloca_0 = memref.alloca() : memref<1xi32>
                memref.store %arg1, %alloca_0[%c0] : memref<1xi32>
        """
        if len(self.indices) < index + 1:
            self.indices += [None] * (index + 1 - len(self.indices))
        if self.indices[index] is None:
            self.indices[index] = arith.ConstantOp.create_index(0, ip=self.index_ip)
        return self.indices[index]

    def loop_or_block(self, instructions: List[wasm.Instruction], is_loop: bool):
        """
        Note on notation: both MLIR and wasm have the concept of a "block". To prevent confusion we use "wasm block" and
        "MLIR block".

        From the wasm spec:
            Unlike with other index spaces, indexing of labels is relative by
            nesting depth, that is, label 0 refers to the innermost structured
            control instruction enclosing the referring branch instruction, while
            increasing indices refer to those farther out. Consequently, labels can
            only be referenced from within the associated structured control
            instruction. This also implies that branches can only be directed
            outwards, “breaking” from the block of the control construct they
            target.  The exact effect depends on that control construct. From the
            wasm spec "In case of block or if it is a forward jump, resuming
            execution after the matching end. In case of loop it is a backward jump
            to the beginning of the loop.
        Example of block behavior:
            # if (test) { consequence } else {alternative }
            # ----
            block [
                block [
                    (test)
                    br_if 0       # Goto 0
                    (alternative)
                    br 1          # Goto 1
                ]                 # Label: 0
                (consequence)
            ]                     # Label: 1
        Examples of loop behavior:
            # while (test) { body }
            # ----
            block [
                loop [            # Label: 0
                    (not test)
                    br_if 1       # Goto 1 (break)
                    (body)
                    br 0,         # Goto 0 (continue)
                ]
            ]                     # Label: 1

            # for(int i = 0; i < n; i++) { body }
            # # which gets converted to # #
            # while (n) {
            #     body;
            #     n --;
            # }
            # ----
            block [
                block [
                    (load n >= 1 onto stack)
                    br_if 0                   # Goto 0
                    br 1                      # Goto 1
                ]                             # Label: 0 (from inner block)
                loop [                        # Label: 0 (from *loop*)
                    (body)
                    (n --)
                    (load n >= 1 onto stack)
                    ('br_if', 0),             # Goto 0 (continue)
                ]
            ]                                 # Label: 1 (from inner block)

        When lowering, A wasm block can be thought of as introducing a label. Some wasm like this:
            block
                ...some stuff...
                br_if 0 (;@1;)
                ...more stuff...
            end
            ... even more stuff ...
        is equivalent to assembly like:
                ...some stuff...
                b.lt target
                ...more stuff...
            target:
                ...even more stuff...
        Inside wasm loop jumps are backwards (continue) rather than the forward jump in wasm blocks (break).
            loop
                ...some stuff...
                br_if 0 (;@1;)
            end
            ... even more stuff ...
        is equivalent to assembly like:
            target:
                ...some stuff...
                b.lt target
                ...even more stuff...
        For simplicity, we always compile a block or a loop into a separate mlir block, even though it isn't strictly
        necessary. Passes can clean up the excess later.

        What about SSA and stuff? Mainly we only need to worry about that when creating jumps, but wasm does have the
        notion ob block arguments. From: https://doi.org/10.1145/3140587.3062363:
            "Conceptually, [wasm] blocks execute like function calls. Each [wasm] block pops its argument values off
            the stack, creates a new stack, pushes the arguments onto the new stack, executes, pops its results off
            the internal stack, and then pushes its results t onto the outer stack."
        The wasm block we're compiling might have arguments if there are jumps into the block with path dependent values
        not stored in wasm locals.
        """
        block = self.block.create_after(tower_runtime.ContextType.get())
        block_after = block.create_after(tower_runtime.ContextType.get())

        # We're not adding an increment_count instruction because this branch
        # doesn't correspond to a wasm instruction. This branch adapts semantics,
        # wasm "falls into" blocks, MLIR jumps into them.
        cf.br([self.last_context], block, ip=self.ip)

        self.move_ip_to_block(block)

        jump_targets = [block_after] + self.jump_targets
        if is_loop:
            jump_targets = [block] + self.jump_targets

        translator = Translator(
            ip=self.ip,
            index_ip=self.index_ip,
            indices=self.indices,
            wasm_locals=self.wasm_locals,
            block=self.block,
            last_context=self.last_context,
            jump_targets=jump_targets,
            functions=self.functions,
        )

        for block_instruction in instructions:
            block_instruction.accept(translator)

        if not is_terminator(instructions[-1]):
            cf.br([self.last_context], block_after, ip=translator.ip)

        self.move_ip_to_block(block_after)

    # VISITOR FUNCTIONS ========================================================

    def visit_loop(self, instruction: wasm.loop) -> None:
        self.loop_or_block(instruction.instructions, is_loop=True)

    def visit_unreachable(self, instruction: wasm.unreachable) -> None:
        pass

    def visit_nop(self, instruction: wasm.nop) -> None:
        pass

    def visit_br(self, instruction: wasm.br) -> None:
        self.last_context = tower_runtime.increment_count(self.last_context, ip=self.ip)
        cf.br([self.last_context], self.jump_targets[instruction.args[0]], ip=self.ip)

    def visit_br_if(self, instruction: wasm.br_if) -> None:
        """
        Note on notation: both MLIR and wasm have the concept of a "block". To prevent confusion we use "wasm block" and
        "MLIR block".

        What do we need to do to compile a conditional branch instruction? We need to create a branch instruction,
        obviously, but we also need to create an MLIR block. That might be a little counterintuitive, let's look at an
        example:
            Example wasm:
                block
                    ...some wasm...
                    br_if 0 (;@1;)                          <---- the jump we're compiling
                    ...more wasm...
                end
                ... even more wasm ...
            Example translation to mlir:
                    ...some wasm translated to mlir...
                    cf.cond_br %0, ^bb2, ^bb1               <---- the jump that gets created
                ^bb1:  // pred: ^bb0
                    ...more wasm translated to mlir...
                ^bb2:  // pred: ^bb0
                    ...even more wasm translated to mlir...
        Because MLIR blocks require straight line code without any jumps, after converting to MLIR "...some wasm..." and
        "...more wasm..." need to be in separate MLIR blocks. In the example, this function will have to create `^bb1`,
        when this function is called `^bb2` should already exist, being created by `visit_block`.

        How about SSA and all that? Do we need to insert phi nodes? As MLIR uses an SSA representation, we have to
        insert a phi node (MLIR actually uses mlir block arguments instead of phi nodes, but it's the same thing) at any
        join point in the control flow graph where the value of a variable (aka something on the wasm stack) is path
        dependent. Since we're messing around with the control flow graph in this function it's important to ask, do we
        need phi nodes?

        Let's look at the cases. Every branch we create in this function will look pretty similar to the example. In the
        example, the new MLIR control flow graph has two edges. One edge jumps to ^bb1 as `...some wasm..." and "...more
        wasm..." are in the same wasm block I call this an "inter-wasm-block jump". One edge jumps to ^bb2 as "...some
        wasm..." and "... even more wasm ..." are in different wasm blocks I call this an "out-of-wasm-block jump".

        inter-wasm-block jump:
            Because a wasm jump can only jump *out* of the wasm block and nothing can jump into a wasm block at an
            arbitrary point, an inter-wasm-block jump _can't_ create a join point where the value of a variable is path
            dependent. The mlir block following an inter-wasm-block jump must have been reached through the mlir block
            before it. In graph theory terminology it the node before the inter-wasm-block jump strictly dominates the
            node after. So we never need phi nodes in the block following an inter-wasm-block jump.

        out-of-wasm-block jumps:
            From: https://doi.org/10.1145/3140587.3062363:
                "Conceptually, [wasm] blocks execute like function calls. Each [wasm] block pops its argument values off
                the stack, creates a new stack, pushes the arguments onto the new stack, executes, pops its results off
                the internal stack, and then pushes its results onto the outer stack."
            By construction, a jump out of a wasm block that creates join points in the control flow graph where the
            value of a variable is path dependent will have wasm jump arguments. We need phi nodes if and only if there
            are block arguments.
        """
        new_block = self.block.create_after(tower_runtime.ContextType.get())
        val = self.pop()
        if val.type != T.bool():
            if val.type != T.i64() and val.type != T.i32():
                raise ValueError("must be int type")
            zero = arith.constant(val.type, 0, ip=self.ip)
            val = arith.cmpi(arith.CmpIPredicate.ne, lhs=val, rhs=zero, ip=self.ip)
        self.last_context = tower_runtime.increment_count(self.last_context, ip=self.ip)
        cf.cond_br(
            val,
            [self.last_context],
            [self.last_context],
            self.jump_targets[instruction.args[0]],
            new_block,
            ip=self.ip,
        )

        self.move_ip_to_block(new_block)

    @wrap_with_increment_and_check_swap
    def visit_local_get(self, instruction: wasm.local_get) -> None:
        self.push(
            memref.load(
                self.wasm_locals[instruction.args[0]],
                self.get_mlir_index(0),
                ip=self.ip,
            )
        )

    @wrap_with_increment_and_check_swap
    def visit_local_set(self, instruction: wasm.local_set) -> None:
        memref.store(
            self.pop(),
            self.wasm_locals[instruction.args[0]],
            self.get_mlir_index(0),
            ip=self.ip,
        )

    @wrap_with_increment_and_check_swap
    def visit_local_tee(self, instruction: wasm.local_tee) -> None:
        memref.store(
            self.peek(),
            self.wasm_locals[instruction.args[0]],
            self.get_mlir_index(0),
            ip=self.ip,
        )

    @wrap_with_increment_and_check_swap
    def visit_i32_const(self, instruction: wasm.i32_const) -> None:
        self.push(arith.constant(T.i32(), instruction.args[0], ip=self.ip))

    @wrap_with_increment_and_check_swap
    def visit_i32_ge_s(self, instruction: wasm.i32_ge_s) -> None:
        self.binary(
            lambda lhs, rhs: arith.cmpi(
                arith.CmpIPredicate.sge, lhs=lhs, rhs=rhs, ip=self.ip
            )
        )

    @wrap_with_increment_and_check_swap
    def visit_i32_add(self, instruction: wasm.i32_add) -> None:
        self.binary(lambda lhs, rhs: arith.addi(lhs=lhs, rhs=rhs, ip=self.ip))

    def visit_block(self, instruction: wasm.block) -> None:
        self.loop_or_block(instruction.instructions, is_loop=False)

    def visit_call(self, instruction: wasm.call) -> None:
        wasm_func = self.functions[instruction.args[0]]
        assert isinstance(wasm_func, ImportedFunction)
        args = list(reversed([self.pop() for _ in range(len(wasm_func.inputs))]))
        result = func.CallOp(wasm_func.mlir_func, args, ip=self.ip)
        if wasm_func.results:
            self.push(result)

    def visit_wasm_return(self, instruction: wasm.wasm_return) -> None:
        func.return_([self.pop()], ip=self.ip)

    @wrap_with_increment_and_check_swap
    def visit_i32_lt_s(self, instruction: wasm.i32_lt_s) -> None:
        self.binary(
            lambda lhs, rhs: arith.cmpi(
                arith.CmpIPredicate.slt, lhs=lhs, rhs=rhs, ip=self.ip
            )
        )


def compile_wasm(wasm_module: wasm.Module) -> mlir.Module:
    """
    expected to be called within a with Context block
    """
    wasm_d.register_dialect()
    tower_runtime.register_dialect()
    wasm_to_mlir_type: Dict[str, mlir.Type] = {"i32": T.i32()}

    with mlir.Location.unknown() as loc:
        mlir_module = mlir.Module.create()
        ip = mlir.InsertionPoint(mlir_module.body)

        # Compile the imports and package up data from wasm.Module related to imported functions for later processing.
        # The binary encoding is convenient for sending the program over wires, but less so for our tasks.
        imported_functions: List[ImportedFunction] = []
        for wasm_import in wasm_module.import_sec:
            if isinstance(wasm_import, wasm.ImportFunction):
                inputs = [
                    wasm_to_mlir_type[t]
                    for t in wasm_module.type_sec[wasm_import.func].param_types
                ]
                results = [
                    wasm_to_mlir_type[t]
                    for t in wasm_module.type_sec[wasm_import.func].result_types
                ]
                imported_functions.append(
                    ImportedFunction(
                        name=wasm_import.name,
                        inputs=inputs,
                        results=results,
                        mlir_func=func.FuncOp(
                            name=wasm_import.name,
                            type=mlir.FunctionType.get(inputs=inputs, results=results),
                            ip=ip,
                            visibility="private",
                        ),
                    )
                )

        # exported_func_idx_to_name is a mapping from function index space to name of exported function. Currently,
        # because we are compiling wasm generated from C code using the "-Wl,--export-all" flag, every function has an
        # exported name. That might not be the case in the future.
        exported_func_idx_to_name: Dict[wasm.func_idx, str] = {
            wasm_func.func: wasm_func.name
            for wasm_func in wasm_module.export_sec
            if isinstance(wasm_func, wasm.ExportFunction)
        }

        # Package up defined functions (aka functions for which this module provides an implementation).
        defined_functions: List[DefinedFunction] = []
        for i, (type_idx, code) in enumerate(
            zip(wasm_module.func_sec, wasm_module.code_sec)
        ):
            func_type = wasm_module.type_sec[type_idx]

            # from the wasm spec: "the index space for functions, tables, memories and globals includes respective
            # imports declared in the same module. The indices of these imports precede the indices of other
            # definitions in the same index space."
            func_idx: wasm.func_idx = wasm.func_idx(len(imported_functions) + i)

            func_name = exported_func_idx_to_name[func_idx]
            # hack for now, I don't really know what "__wasm_call_ctors" does which seems to be  there in every
            # wasm blob compiled from a C program and stuff
            if not func_name.startswith("__"):
                inputs = [wasm_to_mlir_type[t] for t in func_type.param_types]
                results = [wasm_to_mlir_type[t] for t in func_type.result_types]
                defined_functions.append(
                    DefinedFunction(
                        name=func_name, inputs=inputs, results=results, code=code
                    )
                )

        # compile defined functions
        for wasm_func in defined_functions:
            # stitch in additional first argument of type tower_runtime.ctx
            input_types = [tower_runtime.ContextType.get()] + wasm_func.inputs
            mlir_func = func.FuncOp(
                name=wasm_func.name,
                type=mlir.FunctionType.get(
                    inputs=input_types,
                    results=wasm_func.results,
                ),
                ip=ip,
            )
            entry_block = mlir_func.add_entry_block()
            index_ip = mlir.InsertionPoint.at_block_begin(entry_block)
            indices = [arith.ConstantOp.create_index(0, ip=index_ip)]
            ip = mlir.InsertionPoint(entry_block)
            ctx_argument, *function_arguments = mlir_func.entry_block.arguments
            # Create stack storage and store all local variables for a function.
            #
            # From the wasm spec: "The index space for locals is only accessible
            # inside a function and includes the parameters of that function, which
            # precede the local variables." So for a function with 1 param of type
            # i32, 2 params of type i64, 4 locals of type i32, and 2 locals of type
            # f32 the locals are expected to be laid out in this order:
            #   [i32, i64,i64, i32,i32,i32,i32, f32,f32]
            #
            # For simplicity, we'll allocate storage on the stack for everything. Then
            # any local.get becomes a load instruction and a local.set becomes a store
            # instruction. MLIR's mem2reg pass should hopefully clean everything up
            # later.
            wasm_locals: List[mlir.Value] = []
            for arg, mlir_type in zip(function_arguments, wasm_func.inputs):
                stack_allocation = memref.alloca(T.memref(1, mlir_type), [], [], ip=ip)
                memref.store(arg, stack_allocation, indices[0], ip=ip)
                wasm_locals.append(stack_allocation)
            for local in wasm_func.code.locals:
                for _ in range(local.n):
                    stack_allocation = memref.alloca(
                        T.memref(1, wasm_to_mlir_type[local.valtype]),
                        [],
                        [],
                        ip=ip,
                    )
                    wasm_locals.append(stack_allocation)

            # translate the instructions
            translator = Translator(
                wasm_locals=wasm_locals,
                block=entry_block,
                ip=ip,
                indices=indices,
                index_ip=index_ip,
                # from the wasm spec: "the index space for functions, tables, memories and globals includes respective
                # imports declared in the same module. The indices of these imports precede the indices of other
                # definitions in the same index space."
                functions=imported_functions + defined_functions,
                last_context=ctx_argument,
            )
            for instr in wasm_func.code.expr.instructions:
                instr.accept(translator)

            # add return if there is one
            if wasm_func.results:
                func.ReturnOp([translator.pop()], ip=translator.ip)

        return mlir_module


def main():
    parser = argparse.ArgumentParser(description="Translate wasm to mlir")
    parser.add_argument("wasm_file", type=str, help="Wasm file to load")

    prog_args = parser.parse_args()

    with open(prog_args.wasm_file, "rb") as file:
        # decode module
        stream = iter(bytes(file.read()))
        wasm_module = wasm.decode(stream)

        with mlir.Context():
            mlir_module = compile_wasm(wasm_module)
            print(mlir_module)


if __name__ == "__main__":
    main()
