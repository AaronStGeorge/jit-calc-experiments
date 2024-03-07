from dataclasses import dataclass, field
from itertools import islice
from typing import Iterator, Callable, List, Mapping, Optional, Tuple

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
class GlobalType:
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
class Expression:
    """
    https://webassembly.github.io/spec/core/binary/instructions.html#expressions
    """

    instructions: List[Instruction]

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls(parse_instruction_list(stream))


@dataclass
class Global:
    """
    https://webassembly.github.io/spec/core/binary/modules.html#global-section
    """

    globaltype: GlobalType
    expr: Expression

    @classmethod
    def parse(cls, stream: Iterator[bytes]) -> "Global":
        return cls(GlobalType.parse(stream), Expression.parse(stream))


@dataclass
class MemType:
    """
    https://webassembly.github.io/spec/core/binary/types.html#memory-types
    """

    min: int
    max: Optional[int]

    @classmethod
    def parse(cls, stream: Iterator[bytes]) -> "MemType":
        b = next(stream)
        if b == 0x00:
            return MemType(min=parse_unsigned(stream), max=None)
        elif b == 0x01:
            return MemType(min=parse_unsigned(stream), max=parse_unsigned(stream))


@dataclass
class FuncType:
    """
    https://webassembly.github.io/spec/core/binary/types.html#function-types
    """

    param_types: List[str]
    result_types: List[str]

    @classmethod
    def parse(cls, stream: Iterator[bytes]) -> "FuncType":
        magic = bytes(islice(stream, 1))
        if magic != b"\x60":
            raise ValueError("Expected the byte 0x60 in functype")
        param_types = parse_vector(stream, parse_valtype)
        result_types = parse_vector(stream, parse_valtype)
        return cls(param_types, result_types)


@dataclass
class Export:
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
class Local:
    """
    https://webassembly.github.io/spec/core/binary/modules.html#binary-code
    """

    n: int
    valtype: str

    @classmethod
    def parse(cls, stream: Iterator[bytes]) -> "ExportSection":
        return cls(parse_signed(stream), parse_valtype(stream))


@dataclass
class Code:
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
class Module:
    """
    https://webassembly.github.io/spec/core/binary/modules.html#binary-module
    """

    # https://webassembly.github.io/spec/core/binary/modules.html#type-section
    type_sec: List[FuncType] = field(default_factory=list)
    # https://webassembly.github.io/spec/core/binary/modules.html#function-section
    func_sec: List[type_idx] = field(default_factory=list)
    # https://webassembly.github.io/spec/core/binary/modules.html#memory-section
    mem_sec: List[MemType] = field(default_factory=list)
    # https://webassembly.github.io/spec/core/binary/modules.html#global-section
    global_sec: List[Global] = field(default_factory=list)
    # https://webassembly.github.io/spec/core/binary/modules.html#export-section
    export_sec: List[Export] = field(default_factory=list)
    # https://webassembly.github.io/spec/core/binary/modules.html#code-section
    code_sec: List[Code] = field(default_factory=list)

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        header = bytes(islice(stream, 4))
        if header != b"\x00asm":
            raise ValueError("Expected .wasm")
        version = bytes(islice(stream, 4))
        if version != b"\x01\x00\x00\x00":
            raise ValueError("Expected version 1")

        sec_id_to_name_and_parsing_func: Mapping[
            int, Tuple[str, Callable[[Iterator[bytes]], any]]
        ] = {
            1: ("type_sec", FuncType.parse),
            3: ("func_sec", parse_unsigned),
            5: ("mem_sec", MemType.parse),
            6: ("global_sec", Global.parse),
            7: ("export_sec", Export.parse),
            10: ("code_sec", Code.parse),
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

                sec_name, parse_func = sec_id_to_name_and_parsing_func[sec_id]

                sections[sec_name] = parse_vector(stream, parse_func)
            except StopIteration:
                break
        return cls(**sections)


def parse_module(stream: Iterator[bytes]) -> Module:
    return Module.parse(stream)
