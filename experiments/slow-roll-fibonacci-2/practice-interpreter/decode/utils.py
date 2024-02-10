from ast import AST
from itertools import islice
import struct
from typing import Callable, Iterator, List


def parse_unsigned(stream: Iterator[bytes]) -> int:
    """
    All integers are encoded using Little Endian Base 128 (LEB128) variable
    length encoding.

    The basic idea of the encoding is: you grab a byte at a time, that byte
    contains 7 bits of the the encoded number, and one bit that tells you if the
    next byte should be consumed or not.

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
    stream: Iterator[bytes], parsing_function: Callable[[Iterator[bytes]], any]
) -> List[AST]:
    """
    https://webassembly.github.io/spec/core/binary/conventions.html#vectors
    """
    num = parse_unsigned(stream)
    return [parsing_function(stream) for _ in range(num)]


def parse_string(stream) -> str:
    return bytes(parse_vector(stream, next)).decode("utf-8")


def parse_valtype(stream: Iterator[bytes]) -> str:
    """
    https://webassembly.github.io/spec/core/binary/types.html#value-types

    A value type is a num type, vector type, or reference type:
      https://webassembly.github.io/spec/core/binary/types.html#number-types
      https://webassembly.github.io/spec/core/binary/types.html#vector-types
      https://webassembly.github.io/spec/core/binary/types.html#reference-types
    """
    encoding_to_type = {
        0x40: None,
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


def parse_float32(stream: Iterator[bytes]):
    """
    "<" indicates little endian byte order
    "f" indicates 32-bit IEEE 754 floating point (AKA a C float)
    """
    return struct.unpack("<f", bytes(islice(stream, 4)))[0]


def parse_float64(stream):
    """
    "<" indicates little endian byte order
    "l" indicates 64-bit IEEE 754 floating point (AKA a C double)
    """
    return struct.unpack("<d", bytes(islice(stream, 8)))[0]
