"""
Do not edit, this is generated via scripts/hacky_codegen.py!
"""

from dataclasses import dataclass
from typing import Iterator, Mapping, Tuple

from .instructions import Instruction
from .utils import parse_float32, parse_float64, parse_signed, parse_unsigned
from .wasm_types import i32, i64, f32, f64
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
class unreachable(Instruction):
    name: str = "unreachable"

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls()


@dataclass
class nop(Instruction):
    name: str = "nop"

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls()


@dataclass
class br(Instruction):
    args: Tuple[label_idx]
    name: str = "br"

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls((parse_signed(stream)))


@dataclass
class br_if(Instruction):
    args: Tuple[label_idx]
    name: str = "br_if"

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls((parse_signed(stream)))


@dataclass
class local_get(Instruction):
    args: Tuple[local_idx]
    name: str = "local.get"

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls((parse_signed(stream)))


@dataclass
class local_set(Instruction):
    args: Tuple[local_idx]
    name: str = "local.set"

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls((parse_signed(stream)))


@dataclass
class local_tee(Instruction):
    args: Tuple[local_idx]
    name: str = "local.tee"

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls((parse_signed(stream)))


@dataclass
class global_get(Instruction):
    args: Tuple[global_idx]
    name: str = "global.get"

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls((parse_signed(stream)))


@dataclass
class global_set(Instruction):
    args: Tuple[global_idx]
    name: str = "global.set"

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls((parse_signed(stream)))


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


@dataclass
class f32_eq(Instruction):
    name: str = "f32.eq"

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls()


@dataclass
class f32_ne(Instruction):
    name: str = "f32.ne"

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls()


@dataclass
class f32_lt(Instruction):
    name: str = "f32.lt"

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls()


@dataclass
class f32_gt(Instruction):
    name: str = "f32.gt"

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls()


@dataclass
class f32_le(Instruction):
    name: str = "f32.le"

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls()


@dataclass
class f32_ge(Instruction):
    name: str = "f32.ge"

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls()


@dataclass
class f64_eq(Instruction):
    name: str = "f64.eq"

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls()


@dataclass
class f64_ne(Instruction):
    name: str = "f64.ne"

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls()


@dataclass
class f64_lt(Instruction):
    name: str = "f64.lt"

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls()


@dataclass
class f64_gt(Instruction):
    name: str = "f64.gt"

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls()


@dataclass
class f64_le(Instruction):
    name: str = "f64.le"

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls()


@dataclass
class f64_ge(Instruction):
    name: str = "f64.ge"

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls()


@dataclass
class i32_clz(Instruction):
    name: str = "i32.clz"

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls()


@dataclass
class i32_ctz(Instruction):
    name: str = "i32.ctz"

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls()


@dataclass
class i32_popcnt(Instruction):
    name: str = "i32.popcnt"

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls()


@dataclass
class i32_add(Instruction):
    name: str = "i32.add"

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls()


@dataclass
class i32_sub(Instruction):
    name: str = "i32.sub"

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls()


@dataclass
class i32_mul(Instruction):
    name: str = "i32.mul"

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls()


@dataclass
class i32_div_s(Instruction):
    name: str = "i32.div_s"

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls()


@dataclass
class i32_div_u(Instruction):
    name: str = "i32.div_u"

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls()


@dataclass
class i32_rem_s(Instruction):
    name: str = "i32.rem_s"

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls()


@dataclass
class i32_rem_u(Instruction):
    name: str = "i32.rem_u"

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls()


@dataclass
class i32_and(Instruction):
    name: str = "i32.and"

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls()


@dataclass
class i32_or(Instruction):
    name: str = "i32.or"

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls()


@dataclass
class i32_xor(Instruction):
    name: str = "i32.xor"

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls()


@dataclass
class i32_shl(Instruction):
    name: str = "i32.shl"

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls()


@dataclass
class i32_shr_s(Instruction):
    name: str = "i32.shr_s"

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls()


@dataclass
class i32_shr_u(Instruction):
    name: str = "i32.shr_u"

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls()


@dataclass
class i32_rtol(Instruction):
    name: str = "i32.rtol"

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls()


@dataclass
class i32_rtor(Instruction):
    name: str = "i32.rtor"

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls()


id_to_generated_instruction_class: Mapping[int, Instruction] = {
    0x0: unreachable,
    0x1: nop,
    0xC: br,
    0xD: br_if,
    0x20: local_get,
    0x21: local_set,
    0x22: local_tee,
    0x23: global_get,
    0x24: global_set,
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
    0x5B: f32_eq,
    0x5C: f32_ne,
    0x5D: f32_lt,
    0x5E: f32_gt,
    0x5F: f32_le,
    0x60: f32_ge,
    0x61: f64_eq,
    0x62: f64_ne,
    0x63: f64_lt,
    0x64: f64_gt,
    0x65: f64_le,
    0x66: f64_ge,
    0x67: i32_clz,
    0x68: i32_ctz,
    0x69: i32_popcnt,
    0x6A: i32_add,
    0x6B: i32_sub,
    0x6C: i32_mul,
    0x6D: i32_div_s,
    0x6E: i32_div_u,
    0x6F: i32_rem_s,
    0x70: i32_rem_u,
    0x71: i32_and,
    0x72: i32_or,
    0x73: i32_xor,
    0x74: i32_shl,
    0x75: i32_shr_s,
    0x76: i32_shr_u,
    0x77: i32_rtol,
    0x78: i32_rtor,
}
