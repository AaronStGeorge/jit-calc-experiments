from abc import abstractmethod, ABC
from dataclasses import dataclass, field
from itertools import islice
from typing import Iterator, Callable, List, Mapping, Optional, Tuple

from .utils import (
    decode_signed,
    decode_string,
    decode_unsigned,
    decode_valtype,
    decode_vector,
)
from .instructions import Instruction, decode_instruction_list
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
    def decode(cls, stream: Iterator[bytes]):
        valtype = decode_valtype(stream)
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
    def decode(cls, stream: Iterator[bytes]):
        return cls(decode_instruction_list(stream))


@dataclass
class Global:
    """
    https://webassembly.github.io/spec/core/binary/modules.html#global-section
    """

    globaltype: GlobalType
    expr: Expression

    @classmethod
    def decode(cls, stream: Iterator[bytes]) -> "Global":
        return cls(GlobalType.decode(stream), Expression.decode(stream))


@dataclass
class MemType:
    """
    https://webassembly.github.io/spec/core/binary/types.html#memory-types
    """

    min: int
    max: Optional[int]

    @classmethod
    def decode(cls, stream: Iterator[bytes]) -> "MemType":
        b = next(stream)
        if b == 0x00:
            return MemType(min=decode_unsigned(stream), max=None)
        elif b == 0x01:
            return MemType(min=decode_unsigned(stream), max=decode_unsigned(stream))


@dataclass
class Import(ABC):
    """
    https://webassembly.github.io/spec/core/binary/modules.html#binary-importsec
    """

    @property
    @abstractmethod
    def name(self):
        pass

    @property
    @abstractmethod
    def module(self):
        pass

    @classmethod
    def decode(cls, stream: Iterator[bytes]) -> "Import":
        export_id_to_section: Mapping[int, Export] = {
            0: ImportFunction,
            # TODO: implement table
            2: ImportMemory,
        }
        module = decode_string(stream)
        name = decode_string(stream)
        id = next(stream)
        return export_id_to_section[id].decode(module, name, stream)


@dataclass
class ImportFunction(Import):
    _module: str
    _name: str
    func: type_idx

    @property
    def name(self):
        return self._name

    @property
    def module(self):
        return self._module

    @classmethod
    def decode(
        cls, module: str, name: str, stream: Iterator[bytes]
    ) -> "ExportFunction":
        return cls(module, name, decode_signed(stream))


@dataclass
class ImportMemory(Import):
    _module: str
    _name: str
    mem_type: MemType

    @property
    def name(self):
        return self._name

    @property
    def module(self):
        return self._module

    @classmethod
    def decode(
        cls, module: str, name: str, stream: Iterator[bytes]
    ) -> "ExportFunction":
        return cls(module, name, MemType.decode(stream))


@dataclass
class FuncType:
    """
    https://webassembly.github.io/spec/core/binary/types.html#function-types
    """

    param_types: List[str]
    result_types: List[str]

    @classmethod
    def decode(cls, stream: Iterator[bytes]) -> "FuncType":
        magic = bytes(islice(stream, 1))
        if magic != b"\x60":
            raise ValueError("Expected the byte 0x60 in functype")
        param_types = decode_vector(stream, decode_valtype)
        result_types = decode_vector(stream, decode_valtype)
        return cls(param_types, result_types)


class Export(ABC):
    """
    https://webassembly.github.io/spec/core/binary/modules.html#binary-export
    """

    @property
    @abstractmethod
    def name(self):
        pass

    @classmethod
    def decode(cls, stream: Iterator[bytes]) -> "Export":
        export_id_to_section: Mapping[int, Export] = {
            0: ExportFunction,
            1: ExportTable,
            2: ExportMemory,
            3: ExportGlobal,
        }
        name = decode_string(stream)
        id = next(stream)
        return export_id_to_section[id].decode(name, stream)


@dataclass
class ExportFunction(Export):
    _name: str
    ref: func_idx

    @property
    def name(self):
        return self._name

    @classmethod
    def decode(cls, name: str, stream: Iterator[bytes]) -> "ExportFunction":
        return cls(name, decode_signed(stream))


@dataclass
class ExportTable(Export):
    _name: str
    ref: table_idx

    @property
    def name(self):
        return self._name

    @classmethod
    def decode(cls, name: str, stream: Iterator[bytes]) -> "ExportTable":
        return cls(name, decode_signed(stream))


@dataclass
class ExportMemory(Export):
    _name: str
    ref: mem_idx

    @property
    def name(self):
        return self._name

    @classmethod
    def decode(cls, name: str, stream: Iterator[bytes]) -> "ExportMemory":
        return cls(name, decode_signed(stream))


@dataclass
class ExportGlobal(Export):
    _name: str
    ref: global_idx

    @property
    def name(self):
        return self._name

    @classmethod
    def decode(cls, name: str, stream: Iterator[bytes]) -> "ExportGlobal":
        return cls(name, decode_signed(stream))


@dataclass
class Local:
    """
    https://webassembly.github.io/spec/core/binary/modules.html#binary-code
    """

    n: int
    valtype: str

    @classmethod
    def decode(cls, stream: Iterator[bytes]) -> "ExportSection":
        return cls(decode_signed(stream), decode_valtype(stream))


@dataclass
class Code:
    """
    https://webassembly.github.io/spec/core/binary/modules.html#binary-code
    """

    locals: List[Local]
    expr: Expression

    @classmethod
    def decode(cls, stream: Iterator[bytes]) -> "ExportSection":
        size = next(stream)
        return cls(decode_vector(stream, Local.decode), Expression.decode(stream))


@dataclass
class Module:
    """
    https://webassembly.github.io/spec/core/binary/modules.html#binary-module
    """

    # https://webassembly.github.io/spec/core/binary/modules.html#type-section
    type_sec: List[FuncType] = field(default_factory=list)
    # https://webassembly.github.io/spec/core/binary/modules.html#binary-importsec
    import_sec: List[Import] = field(default_factory=list)
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
    def decode(cls, stream: Iterator[bytes]):
        header = bytes(islice(stream, 4))
        if header != b"\x00asm":
            raise ValueError("Expected .wasm")
        version = bytes(islice(stream, 4))
        if version != b"\x01\x00\x00\x00":
            raise ValueError("Expected version 1")

        sec_id_to_name_and_parsing_func: Mapping[
            int, Tuple[str, Callable[[Iterator[bytes]], any]]
        ] = {
            1: ("type_sec", FuncType.decode),
            2: ("import_sec", Import.decode),
            3: ("func_sec", decode_unsigned),
            5: ("mem_sec", MemType.decode),
            6: ("global_sec", Global.decode),
            7: ("export_sec", Export.decode),
            10: ("code_sec", Code.decode),
        }

        sections = {}
        while True:
            try:
                sec_id = next(stream)
                size = decode_unsigned(stream)
                # just skip custom sections for now
                if sec_id == 0:
                    stream = islice(stream, size, None)
                    continue

                sec_name, decode_func = sec_id_to_name_and_parsing_func[sec_id]

                sections[sec_name] = decode_vector(stream, decode_func)
            except StopIteration:
                break
        return cls(**sections)
