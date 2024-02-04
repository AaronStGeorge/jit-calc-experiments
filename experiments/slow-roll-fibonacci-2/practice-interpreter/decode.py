from itertools import islice
from typing import Callable, Iterator, List, NamedTuple, Optional


def parse_unsigned(stream: Iterator[bytes]) -> int:
    """
    All integers are encoded using Little Endian Base 128 (LEB128) variable
    length encoding.

    https://webassembly.github.io/spec/core/binary/values.html#integers
    """
    value = 0
    shift = 0
    while True:
        b = next(stream)
        # 0x7f = 0b_0111_1111
        value |= 0x7F & b
        value <<= shift
        shift += 7
        # 0x80 = 0b_1000_0000
        if not (b & 0x80):
            break

    return value


def parse_vector(
    stream: Iterator[bytes], parsing_function: Callable[[Iterator[bytes]], None]
) -> List:
    """
    https://webassembly.github.io/spec/core/binary/conventions.html#vectors
    """
    num = parse_unsigned(stream)
    return [parsing_function(stream) for _ in range(num)]


def parse_valtype(stream: Iterator[bytes]):
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


GlobalType = NamedTuple("GlobalType", valtype=str, mut=str)


def parse_globaltype(stream: Iterator[bytes]):
    """
    https://webassembly.github.io/spec/core/binary/types.html#global-types
    """
    valtype = parse_valtype(stream)
    b = bytes(islice(stream, 1))
    if b == b"\00":
        return GlobalType(valtype, "const")
    elif b == b"\01":
        return GlobalType(valtype, "var")



Global = NamedTuple("Global", globaltype=GlobalType, expr=None)


def parse_global(stream: Iterator[bytes]):
    """
    https://webassembly.github.io/spec/core/binary/modules.html#global-section
    """
    globaltype = parse_globaltype(stream)
    print("boop")


MemoryType = NamedTuple("MemoryType", min=int, max=Optional[int])


def parse_memory_type(stream: Iterator[bytes]):
    """
    https://webassembly.github.io/spec/core/binary/types.html#memory-types
    """
    b = bytes(islice(stream, 1))
    if b == b"\00":
        return MemoryType(min=parse_unsigned(stream), max=None)
    elif b == b"\01":
        return MemoryType(min=parse_unsigned(stream), max=parse_unsigned(stream))


FunctionType = NamedTuple("FunctionType", param_types=List, result_types=List)


def parse_func_type(stream: Iterator[bytes]):
    """
    https://webassembly.github.io/spec/core/binary/types.html#function-types
    """
    magic = bytes(islice(stream, 1))
    if magic != b"\x60":
        raise ValueError("Expected the byte 0x60 in functype")
    param_types = parse_vector(stream, parse_valtype)
    result_types = parse_vector(stream, parse_valtype)
    return FunctionType(param_types, result_types)


def parse_section(stream: Iterator[bytes]):
    """
    https://webassembly.github.io/spec/core/binary/modules.html#sections
    """
    section_id_to_parsing_fn = {
        # https://webassembly.github.io/spec/core/binary/modules.html#type-section
        1: lambda stream: parse_vector(stream, parse_func_type),
        # https://webassembly.github.io/spec/core/binary/modules.html#function-section
        3: lambda stream: parse_vector(stream, parse_unsigned),
        # https://webassembly.github.io/spec/core/binary/modules.html#memory-section
        5: lambda stream: parse_vector(stream, parse_memory_type),
        # https://webassembly.github.io/spec/core/binary/modules.html#global-section
        6: lambda stream: parse_vector(stream, parse_global),
    }
    sec_id = next(stream)
    parse_unsigned(stream)  # size (unused for now)
    if sec_id not in section_id_to_parsing_fn:
        raise ValueError("Unknown Section ID")
    return section_id_to_parsing_fn[sec_id](stream)


# So far this is a straight forward clone of https://github.com/dabeaz/wadze
if __name__ == "__main__":
    with open("add.wasm", "rb") as file:
        stream = iter(bytes(file.read()))
        header = bytes(islice(stream, 4))
        if header != b"\x00asm":
            raise ValueError("Expected .wasm")
        version = bytes(islice(stream, 4))
        if version != b"\x01\x00\x00\x00":
            raise ValueError("Expected version 1")

        one = parse_section(stream)
        two = parse_section(stream)
        three = parse_section(stream)
        # four = parse_section(stream)
        # sec_id = next(stream)

        print(stream)
