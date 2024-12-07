import struct
from ast import AST
from itertools import islice
from typing import Callable, Iterator, List, TypeVar


def decode_unsigned(stream: Iterator[bytes]) -> int:
    """
    All integers are encoded using Little Endian Base 128 (LEB128) variable
    length encoding.

    The basic idea of the encoding is: you grab a byte at a time, that byte
    contains 7 bits of the encoded number, and one bit that tells you if the
    next byte should be consumed or not.

    0b_XXXX_XXXX
       ^^      ^
       ||      |
       | ------
       |   |
       |   > 7 bits of encoded number
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


def decode_signed(stream: Iterator[bytes]) -> int:
    """
    Exactly like decode_unsigned byt with sign extension
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


def decode_vector(
    stream: Iterator[bytes], decoding_function: Callable[[Iterator[bytes]], any]
) -> List[AST]:
    """
    https://webassembly.github.io/spec/core/binary/conventions.html#vectors
    """
    num = decode_unsigned(stream)
    return [decoding_function(stream) for _ in range(num)]


def decode_string(stream) -> str:
    return bytes(decode_vector(stream, next)).decode("utf-8")


def decode_valtype(stream: Iterator[bytes]) -> str:
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


def decode_float32(stream: Iterator[bytes]):
    """
    "<" indicates little endian byte order
    "f" indicates 32-bit IEEE 754 floating point (AKA a C float)
    """
    return struct.unpack("<f", bytes(islice(stream, 4)))[0]


def decode_float64(stream):
    """
    "<" indicates little endian byte order
    "l" indicates 64-bit IEEE 754 floating point (AKA a C double)
    """
    return struct.unpack("<d", bytes(islice(stream, 8)))[0]


class BytesIterator(Iterator[bytes]):
    """This exists, vs just using python's `iter` function on a bytes object, to
    provide a nice __str__ method for debugging."""

    def __init__(self, data: bytes):
        self.data = data
        self.index = 0

    def __iter__(self):
        return self

    def __next__(self) -> int:
        if self.index >= len(self.data):
            raise StopIteration
        value = self.data[self.index]
        self.index += 1
        return value

    def __str__(self):
        return str(self.data[self.index :])


T = TypeVar("T")


class CheckedIterator(Iterator[T]):
    """Checks that expected_count items are pulled from wrapped iterator."""

    def __init__(self, iterator: Iterator[T], expected_count: int):
        self.iterator = iterator
        self.count = 0
        self.expected_count = expected_count

    def __iter__(self):
        return self

    def __next__(self) -> T:
        self.count += 1
        if self.count > self.expected_count:
            raise ValueError(
                f"Expected {self.expected_count} items, but iterated over {self.count} items."
            )
        return next(self.iterator)

    def __enter__(self):
        self.count = 0
        return self

    def __exit__(self, exc_type, _, __) -> bool:
        if exc_type is None:
            # No exceptions occurred
            if self.count != self.expected_count:
                raise ValueError(
                    f"Expected {self.expected_count} items, but iterated over {self.count} items."
                )
        else:
            # An exception occurred; re-raise it
            return False
