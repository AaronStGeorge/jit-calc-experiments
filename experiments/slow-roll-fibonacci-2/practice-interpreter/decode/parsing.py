from abc import abstractmethod
from dataclasses import dataclass, field
from itertools import islice
from typing import Iterator, List, Mapping, Optional, Tuple

from .ast import AST
from .utils import (
    parse_signed,
    parse_string,
    parse_unsigned,
    parse_valtype,
    parse_vector,
)
from .instructions import Instruction, parse_instruction_list
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
    https://webassembly.github.io/spec/core/binary/instructions.html#expressions
    """

    instructions: List[Instruction]

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls(parse_instruction_list(stream))


@dataclass
class Global(AST):
    """
    https://webassembly.github.io/spec/core/binary/modules.html#global-section
    """

    globaltype: GlobalType
    expr: Expression

    @classmethod
    def parse(cls, stream: Iterator[bytes]) -> "Global":
        return cls(GlobalType.parse(stream), Expression.parse(stream))


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


class Section(AST):
    """
    https://webassembly.github.io/spec/core/binary/modules.html#sections
    """

    @classmethod
    @abstractmethod
    def parse(cls, sec_id: int, stream: Iterator[bytes]):
        section_id_to_section: Mapping[int, Section] = {
            1: TypeSection,
            3: FunctionSection,
            5: MemorySection,
            6: GlobalSection,
            7: ExportSection,
            10: CodeSection,
        }
        if sec_id not in section_id_to_section:
            raise ValueError("Unknown Section ID")
        return section_id_to_section[sec_id].parse(stream)


@dataclass
class TypeSection(Section):
    """
    https://webassembly.github.io/spec/core/binary/modules.html#type-section
    """

    function_types: List[FunctionType] = field(default_factory=list)

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls(parse_vector(stream, FunctionType.parse))


@dataclass
class MemorySection(Section):
    """
    https://webassembly.github.io/spec/core/binary/modules.html#memory-section
    """

    memories: List[MemoryType] = field(default_factory=[])

    @classmethod
    def parse(cls, stream: Iterator[bytes]) -> "MemorySection":
        return cls(parse_vector(stream, MemoryType.parse))


@dataclass
class FunctionSection(Section):
    """
    https://webassembly.github.io/spec/core/binary/modules.html#function-section
    """

    function_indices: List[type_idx] = field(default_factory=list)

    @classmethod
    def parse(cls, stream: Iterator[bytes]) -> "FunctionSection":
        return cls(parse_vector(stream, parse_unsigned))


@dataclass
class GlobalSection(Section):
    """
    https://webassembly.github.io/spec/core/binary/modules.html#global-section
    """

    globals: List[Global] = field(default_factory=list)

    @classmethod
    def parse(cls, stream: Iterator[bytes]) -> "GlobalSection":
        return cls(parse_vector(stream, Global.parse))


@dataclass
class Export(Section):
    """
    https://webassembly.github.io/spec/core/binary/modules.html#binary-export
    """

    @classmethod
    def parse(cls, stream: Iterator[bytes]) -> "Export":
        export_id_to_section: Mapping[int, Export] = {
            0: ExportFunction,
            1: ExportTable,
            2: ExportMemory,
            3: ExportGlobal,
        }
        name = parse_string(stream)
        id = next(stream)
        return export_id_to_section[id].parse(name, stream)


@dataclass
class ExportFunction(Export):
    name: str
    ref: func_idx

    @classmethod
    def parse(cls, name: str, stream: Iterator[bytes]) -> "ExportFunction":
        return cls(name, parse_signed(stream))


@dataclass
class ExportTable(Export):
    name: str
    ref: table_idx

    @classmethod
    def parse(cls, name: str, stream: Iterator[bytes]) -> "ExportTable":
        return cls(name, parse_signed(stream))


@dataclass
class ExportMemory(Export):
    name: str
    ref: mem_idx

    @classmethod
    def parse(cls, name: str, stream: Iterator[bytes]) -> "ExportMemory":
        return cls(name, parse_signed(stream))


@dataclass
class ExportGlobal(Export):
    name: str
    ref: global_idx

    @classmethod
    def parse(cls, name: str, stream: Iterator[bytes]) -> "ExportGlobal":
        return cls(name, parse_signed(stream))


@dataclass
class ExportSection(Section):
    """
    https://webassembly.github.io/spec/core/binary/modules.html#export-section
    """

    exports: List[Export] = field(default_factory=list)

    @classmethod
    def parse(cls, stream: Iterator[bytes]) -> "ExportSection":
        return cls(parse_vector(stream, Export.parse))


@dataclass
class Local(AST):
    """
    https://webassembly.github.io/spec/core/binary/modules.html#binary-code
    """

    n: int
    valtype: str

    @classmethod
    def parse(cls, stream: Iterator[bytes]) -> "ExportSection":
        return cls(parse_signed(stream), parse_valtype(stream))


@dataclass
class Code(AST):
    """
    https://webassembly.github.io/spec/core/binary/modules.html#binary-code
    """

    locals: List
    instructions: Expression

    @classmethod
    def parse(cls, stream: Iterator[bytes]) -> "ExportSection":
        size = next(stream)
        return cls(parse_vector(stream, Local.parse), Expression.parse(stream))


@dataclass
class CodeSection(Section):
    """
    https://webassembly.github.io/spec/core/binary/modules.html#code-section
    """

    code: List[Code] = field(default_factory=list)

    @classmethod
    def parse(cls, stream: Iterator[bytes]) -> "ExportSection":
        return cls(parse_vector(stream, Code.parse))


@dataclass
class Module(AST):
    """
    https://webassembly.github.io/spec/core/binary/modules.html#binary-module
    """

    type_section: TypeSection = field(default_factory=TypeSection)
    function_section: FunctionSection = field(default_factory=FunctionSection)
    memory_section: MemorySection = field(default_factory=MemorySection)
    global_section: GlobalSection = field(default_factory=GlobalSection)
    export_section: ExportSection = field(default_factory=ExportSection)
    code_section: CodeSection = field(default_factory=CodeSection)

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        header = bytes(islice(stream, 4))
        if header != b"\x00asm":
            raise ValueError("Expected .wasm")
        version = bytes(islice(stream, 4))
        if version != b"\x01\x00\x00\x00":
            raise ValueError("Expected version 1")

        section_id_to_name: Mapping[int, str] = {
            1: "type_section",
            3: "function_section",
            5: "memory_section",
            6: "global_section",
            7: "export_section",
            10: "code_section",
        }

        sections = {}
        while True:
            try:
                sec_id = next(stream)
                size = parse_unsigned(stream)
                # just skip custom sections for now
                if sec_id == 0:
                    stream = islice(stream, size, None)
                    continue

                sections[section_id_to_name[sec_id]] = Section.parse(sec_id, stream)
            except StopIteration:
                break
        return cls(**sections)


def parse_module(stream: Iterator[bytes]) -> Module:
    return Module.parse(stream)
