from typing import Union
from numpy import uint32, int32, int64, float32, float64


i32 = int32
u32 = uint32
s32 = int32

i64 = int64
u64 = int64
s64 = int64

f32 = float32
f64 = float64

WasmType = Union[
    i32,
    u32,
    s32,
    i64,
    u64,
    s64,
    f32,
    f64,
]