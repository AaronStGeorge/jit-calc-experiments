"""
Do not edit, this is generated via scripts/hacky_codegen.py!
"""

from dataclasses import dataclass
from typing import Iterator, Mapping, Tuple

from .instructions import Instruction
from .utils import parse_float32, parse_float64, parse_signed, parse_unsigned
from .wasm_types import i32, i64, f32, f64


@dataclass
class i32_const(Instruction):
    args: Tuple[i32]
    name: str = "i32.const"

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls((parse_signed(stream)))


@dataclass
class i64_const(Instruction):
    args: Tuple[i64]
    name: str = "i64.const"

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls((parse_signed(stream)))


@dataclass
class f32_const(Instruction):
    args: Tuple[f32]
    name: str = "f32.const"

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls((parse_float32(stream)))


@dataclass
class f64_const(Instruction):
    args: Tuple[f64]
    name: str = "f64.const"

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls((parse_float64(stream)))


@dataclass
class i32_eqz(Instruction):
    name: str = "i32.eqz"

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls()


@dataclass
class i32_eq(Instruction):
    name: str = "i32.eq"

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls()


@dataclass
class i32_ne(Instruction):
    name: str = "i32.ne"

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls()


@dataclass
class i32_lt_s(Instruction):
    name: str = "i32.lt_s"

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls()


@dataclass
class i32_lt_u(Instruction):
    name: str = "i32.lt_u"

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls()


@dataclass
class i32_gt_s(Instruction):
    name: str = "i32.gt_s"

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls()


@dataclass
class i32_gt_u(Instruction):
    name: str = "i32.gt_u"

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls()


@dataclass
class i32_le_s(Instruction):
    name: str = "i32.le_s"

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls()


@dataclass
class i32_le_u(Instruction):
    name: str = "i32.le_u"

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls()


@dataclass
class i32_ge_s(Instruction):
    name: str = "i32.ge_s"

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls()


@dataclass
class i32_ge_u(Instruction):
    name: str = "i32.ge_u"

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls()


@dataclass
class i64_eqz(Instruction):
    name: str = "i64.eqz"

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls()


@dataclass
class i64_eq(Instruction):
    name: str = "i64.eq"

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls()


@dataclass
class i64_ne(Instruction):
    name: str = "i64.ne"

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls()


@dataclass
class i64_lt_s(Instruction):
    name: str = "i64.lt_s"

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls()


@dataclass
class i64_lt_u(Instruction):
    name: str = "i64.lt_u"

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls()


@dataclass
class i64_gt_s(Instruction):
    name: str = "i64.gt_s"

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls()


@dataclass
class i64_gt_u(Instruction):
    name: str = "i64.gt_u"

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls()


@dataclass
class i64_le_s(Instruction):
    name: str = "i64.le_s"

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls()


@dataclass
class i64_le_u(Instruction):
    name: str = "i64.le_u"

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls()


@dataclass
class i64_ge_s(Instruction):
    name: str = "i64.ge_s"

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls()


@dataclass
class i64_ge_u(Instruction):
    name: str = "i64.ge_u"

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls()


id_to_generated_instruction_class: Mapping[int, Instruction] = {
    0x41: i32_const,
    0x42: i64_const,
    0x43: f32_const,
    0x44: f64_const,
    0x45: i32_eqz,
    0x46: i32_eq,
    0x47: i32_ne,
    0x48: i32_lt_s,
    0x49: i32_lt_u,
    0x4A: i32_gt_s,
    0x4B: i32_gt_u,
    0x4C: i32_le_s,
    0x4D: i32_le_u,
    0x4E: i32_ge_s,
    0x4F: i32_ge_u,
    0x50: i64_eqz,
    0x51: i64_eq,
    0x52: i64_ne,
    0x53: i64_lt_s,
    0x54: i64_lt_u,
    0x55: i64_gt_s,
    0x56: i64_gt_u,
    0x57: i64_le_s,
    0x58: i64_le_u,
    0x59: i64_ge_s,
    0x5A: i64_ge_u,
}
