from dataclasses import dataclass
from typing import Generic, Iterator, List, Mapping, Tuple, Dict, TypeVar
from abc import ABC, abstractmethod

from .utils import (
    parse_float32,
    parse_float64,
    parse_signed,
    parse_unsigned,
    parse_valtype,
)
from .wasm_types import i32, i64, f32, f64
from .indices import (
    type_idx,
    func_idx,
    table_idx,
    mem_idx,
    global_idx,
    elem_idx,
    data_idx,
    local_idx,
    label_idx,
)

T = TypeVar("T")


class Instruction(ABC):
    """
    https://webassembly.github.io/spec/core/binary/instructions.html#instructions
    """

    @property
    @abstractmethod
    def name(self) -> str:
        """
        Name of instruction
        """

    @abstractmethod
    def accept(self, visitor: "Visitor[T]") -> T:
        pass

    @classmethod
    def parse(cls, op_code: int, stream: Iterator[bytes]):
        op_code_to_class: Mapping[int, Instruction] = {
            **op_code_to_generated_class(),
            0x02: block,
            0x03: loop,
        }
        return op_code_to_class[op_code].parse(stream)


def parse_instruction_list(stream: Iterator[bytes]) -> List[Instruction]:
    """
    Parse instructions until op_code 0x0B comes up. Wasm spec uses this in
    several places.
    """
    instructions: List[Instruction] = []
    while True:
        op_code = next(stream)
        if op_code == 0x0B:
            break
        instructions.append(Instruction.parse(op_code, stream))
    return instructions


@dataclass
class block(Instruction):
    block_type: str
    instructions: List[Instruction]
    name: str = "block"

    def accept(self, visitor: "Visitor[T]") -> T:
        return visitor.visit_block(self)

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls(parse_valtype(stream), parse_instruction_list(stream))


@dataclass
class loop(Instruction):
    block_type: str
    instructions: List[Instruction]
    name: str = "loop"

    def accept(self, visitor: "Visitor[T]") -> T:
        return visitor.visit_loop(self)

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls(parse_valtype(stream), parse_instruction_list(stream))


class ManualVisitor(ABC, Generic[T]):

    @abstractmethod
    def visit_block(self, instruction: block) -> T:
        pass

    @abstractmethod
    def visit_loop(self, instruction: loop) -> T:
        pass


# =============================================================================
# Everything "GEN START" and "GEN STOP" is generated with hacky_codegen.py in
# the scripts directory.
#
# This sucks, but python really wants all code for a module to be in the same
# file so it just ends up being easier to dump generated code in this file vs
# deal with circular imports for a separate one.
# =============================================================================

##GEN_START##


@dataclass
class unreachable(Instruction):
    name: str = "unreachable"

    def accept(self, visitor: "Visitor[T]") -> T:
        return visitor.visit_unreachable(self)

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls()


@dataclass
class nop(Instruction):
    name: str = "nop"

    def accept(self, visitor: "Visitor[T]") -> T:
        return visitor.visit_nop(self)

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls()


@dataclass
class br(Instruction):
    args: Tuple[label_idx]
    name: str = "br"

    def accept(self, visitor: "Visitor[T]") -> T:
        return visitor.visit_br(self)

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls((parse_signed(stream),))


@dataclass
class br_if(Instruction):
    args: Tuple[label_idx]
    name: str = "br_if"

    def accept(self, visitor: "Visitor[T]") -> T:
        return visitor.visit_br_if(self)

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls((parse_signed(stream),))


@dataclass
class local_get(Instruction):
    args: Tuple[local_idx]
    name: str = "local.get"

    def accept(self, visitor: "Visitor[T]") -> T:
        return visitor.visit_local_get(self)

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls((parse_signed(stream),))


@dataclass
class local_set(Instruction):
    args: Tuple[local_idx]
    name: str = "local.set"

    def accept(self, visitor: "Visitor[T]") -> T:
        return visitor.visit_local_set(self)

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls((parse_signed(stream),))


@dataclass
class local_tee(Instruction):
    args: Tuple[local_idx]
    name: str = "local.tee"

    def accept(self, visitor: "Visitor[T]") -> T:
        return visitor.visit_local_tee(self)

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls((parse_signed(stream),))


@dataclass
class i32_const(Instruction):
    args: Tuple[i32]
    name: str = "i32.const"

    def accept(self, visitor: "Visitor[T]") -> T:
        return visitor.visit_i32_const(self)

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls((parse_signed(stream),))


@dataclass
class i32_ge_s(Instruction):
    name: str = "i32.ge_s"

    def accept(self, visitor: "Visitor[T]") -> T:
        return visitor.visit_i32_ge_s(self)

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls()


@dataclass
class i32_add(Instruction):
    name: str = "i32.add"

    def accept(self, visitor: "Visitor[T]") -> T:
        return visitor.visit_i32_add(self)

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls()


def op_code_to_generated_class() -> Dict[int, Instruction]:
    return {
        0x0: unreachable,
        0x1: nop,
        0xC: br,
        0xD: br_if,
        0x20: local_get,
        0x21: local_set,
        0x22: local_tee,
        0x41: i32_const,
        0x4E: i32_ge_s,
        0x6A: i32_add,
    }


class GeneratedVisitor(ABC, Generic[T]):

    @abstractmethod
    def visit_unreachable(self, instruction: unreachable) -> T:
        pass

    @abstractmethod
    def visit_nop(self, instruction: nop) -> T:
        pass

    @abstractmethod
    def visit_br(self, instruction: br) -> T:
        pass

    @abstractmethod
    def visit_br_if(self, instruction: br_if) -> T:
        pass

    @abstractmethod
    def visit_local_get(self, instruction: local_get) -> T:
        pass

    @abstractmethod
    def visit_local_set(self, instruction: local_set) -> T:
        pass

    @abstractmethod
    def visit_local_tee(self, instruction: local_tee) -> T:
        pass

    @abstractmethod
    def visit_i32_const(self, instruction: i32_const) -> T:
        pass

    @abstractmethod
    def visit_i32_ge_s(self, instruction: i32_ge_s) -> T:
        pass

    @abstractmethod
    def visit_i32_add(self, instruction: i32_add) -> T:
        pass


##GEN_STOP##


class Visitor(ManualVisitor[T], GeneratedVisitor[T]):
    """ """
