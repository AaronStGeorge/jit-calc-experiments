from .module_and_sections import (
    GlobalType,
    Expression,
    Global,
    MemType,
    FuncType,
    Export,
    ExportFunction,
    ExportTable,
    ExportMemory,
    ExportGlobal,
    Local,
    Code,
    Module,
)
from .wasm_types import *
from .instructions import *


def decode(stream: Iterator[bytes]) -> Module:
    """
    decode encoded wasm module
    """
    return Module.decode(stream)
