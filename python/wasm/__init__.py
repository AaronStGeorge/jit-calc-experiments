from .module_and_sections import *
from .wasm_types import *
from .instructions import *


def decode(module: bytes) -> Module:
    """
    decode encoded wasm module
    """
    return Module.decode(module)
