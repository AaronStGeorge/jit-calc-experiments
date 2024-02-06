from abc import ABC, abstractmethod
from dataclasses import dataclass
from itertools import islice
from typing import Callable, Iterator, List, Mapping, Optional, Tuple

i32 = int


class AST(ABC):
    @classmethod
    @abstractmethod
    def parse(cls, stream: Iterator[bytes]) -> "AST":
        pass


class Instruction(AST):
    """
    https://webassembly.github.io/spec/core/binary/instructions.html#instructions
    """

    @property
    @abstractmethod
    def name(self) -> str:
        pass

    @classmethod
    @abstractmethod
    def parse(cls, stream: Iterator[bytes]):
        op_code_to_class: Mapping[int, Instruction] = {0x41: i32_const}
        b = next(stream)
        instr = op_code_to_class[b].parse(stream)
        return instr


@dataclass
class i32_const(Instruction):
    """
    https://webassembly.github.io/spec/core/binary/instructions.html#numeric-instructions
    """

    args: Tuple[i32]
    name: str = "i32.const"

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        arg = parse_signed(stream)
        return cls((arg))


def parse_unsigned(stream: Iterator[bytes]) -> int:
    """
    All integers are encoded using Little Endian Base 128 (LEB128) variable
    length encoding.

    The basic idea of the encoding is:
    0b_XXXX_XXXX
       ^^      ^
       ||      |
       | ------
       |    |
       |    > 7 bits of encoded number
       > indicates if this is the last "chunk" or not

    https://webassembly.github.io/spec/core/binary/values.html#integers
    """
    value = 0
    shift = 0
    while True:
        b = next(stream)
        # 0x7f = 0b_0111_1111
        value |= (0x7F & b) << shift
        shift += 7
        # 0x80 = 0b_1000_0000
        if not b & 0x80:
            break

    return value


def parse_signed(stream: Iterator[bytes]) -> int:
    """
    Exactly like parse_unsigned byt with sign extension
    """
    value = 0
    shift = 0
    while True:
        b = next(stream)
        value |= (0x7F & b) << shift
        shift += 7
        if not b & 0x80:
            break

    # 0x40 = 0b_0100_0000
    if b & 0x40:
        # sign extension
        value |= ~0 << shift
    return value


def parse_vector(
    stream: Iterator[bytes], parsing_function: Callable[[Iterator[bytes]], AST]
) -> List:
    """
    https://webassembly.github.io/spec/core/binary/conventions.html#vectors
    """
    num = parse_unsigned(stream)
    return [parsing_function(stream) for _ in range(num)]


def parse_valtype(stream: Iterator[bytes]) -> str:
    """
    https://webassembly.github.io/spec/core/binary/types.html#value-types

    A value type is a num type, vector type, or reference type:
      https://webassembly.github.io/spec/core/binary/types.html#number-types
      https://webassembly.github.io/spec/core/binary/types.html#vector-types
      https://webassembly.github.io/spec/core/binary/types.html#reference-types
    """
    encoding_to_type = {
        0x7F: "i32",
        0x7E: "i64",
        0x7D: "f32",
        0x7C: "f64",
        0x7B: "v128",
        0x70: "funcref",
        0x6F: "externref",
    }
    b = next(stream)
    if b not in encoding_to_type:
        raise ValueError("Unknown type")
    return encoding_to_type[b]


@dataclass
class GlobalType(AST):
    """
    https://webassembly.github.io/spec/core/binary/types.html#global-types
    """

    valtype: str
    mut: str

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        valtype = parse_valtype(stream)
        b = next(stream)
        if b == 0x00:
            return cls(valtype, "const")
        elif b == 0x01:
            return cls(valtype, "var")
        else:
            raise ValueError("Unknown global mut")


@dataclass
class Expression(AST):
    """
    https://webassembly.github.io/spec/core/binary/instructions.html#instructions
    """

    instrs: List[Instruction]

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        pass


@dataclass
class Global(AST):
    """
    https://webassembly.github.io/spec/core/binary/modules.html#global-section
    """

    globaltype: GlobalType
    expr: Expression

    @classmethod
    def parse(cls, stream: Iterator[bytes]) -> "Global":
        globaltype = GlobalType.parse(stream)
        instr = Instruction.parse(stream)
        print("boop")


@dataclass
class MemoryType(AST):
    """
    https://webassembly.github.io/spec/core/binary/types.html#memory-types
    """

    min: int
    max: Optional[int]

    @classmethod
    def parse(cls, stream: Iterator[bytes]) -> "MemoryType":
        b = next(stream)
        if b == 0x00:
            return MemoryType(min=parse_unsigned(stream), max=None)
        elif b == 0x01:
            return MemoryType(min=parse_unsigned(stream), max=parse_unsigned(stream))


@dataclass
class FunctionType(AST):
    """
    https://webassembly.github.io/spec/core/binary/types.html#function-types
    """

    param_types: List[str]
    result_types: List[str]

    @classmethod
    def parse(cls, stream: Iterator[bytes]) -> "FunctionType":
        magic = bytes(islice(stream, 1))
        if magic != b"\x60":
            raise ValueError("Expected the byte 0x60 in functype")
        param_types = parse_vector(stream, parse_valtype)
        result_types = parse_vector(stream, parse_valtype)
        return cls(param_types, result_types)


TypeIdx = int


class Section(AST):
    """
    https://webassembly.github.io/spec/core/binary/modules.html#sections
    """

    @classmethod
    @abstractmethod
    def parse(cls, stream: Iterator[bytes]):
        section_id_to_section: Mapping[int, Section] = {
            1: TypeSection,
            3: FunctionSection,
            5: MemorySection,
            6: GlobalSection,
        }
        sec_id = next(stream)
        parse_unsigned(stream)  # size (unused for now)
        if sec_id not in section_id_to_section:
            raise ValueError("Unknown Section ID")
        return section_id_to_section[sec_id].parse(stream)


@dataclass
class TypeSection(Section):
    """
    https://webassembly.github.io/spec/core/binary/modules.html#type-section
    """

    function_types: List[FunctionType]

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls(parse_vector(stream, FunctionType.parse))


@dataclass
class MemorySection(Section):
    """
    https://webassembly.github.io/spec/core/binary/modules.html#memory-section
    """

    memories: List[MemoryType]

    @classmethod
    def parse(cls, stream: Iterator[bytes]) -> "MemorySection":
        return cls(parse_vector(stream, MemoryType.parse))


@dataclass
class FunctionSection(Section):
    """
    https://webassembly.github.io/spec/core/binary/modules.html#function-section
    """

    function_indices: List[TypeIdx]

    @classmethod
    def parse(cls, stream: Iterator[bytes]) -> "FunctionSection":
        return cls(parse_vector(stream, parse_unsigned))


@dataclass
class GlobalSection(Section):
    """
    https://webassembly.github.io/spec/core/binary/modules.html#global-section
    """

    globals: List[Global]

    @classmethod
    def parse(cls, stream: Iterator[bytes]) -> "GlobalSection":
        return cls(parse_vector(stream, Global.parse))


class Module(AST):
    """
    https://webassembly.github.io/spec/core/binary/modules.html#binary-module
    """

    sections: List[Section]

    @classmethod
    @abstractmethod
    def parse(cls, stream: Iterator[bytes]):
        header = bytes(islice(stream, 4))
        if header != b"\x00asm":
            raise ValueError("Expected .wasm")
        version = bytes(islice(stream, 4))
        if version != b"\x01\x00\x00\x00":
            raise ValueError("Expected version 1")

        one = Section.parse(stream)
        two = Section.parse(stream)
        three = Section.parse(stream)
        four = Section.parse(stream)
        sec_id = next(stream)


def parse_module(stream: Iterator[bytes]) -> Module:
    return Module.parse(stream)
