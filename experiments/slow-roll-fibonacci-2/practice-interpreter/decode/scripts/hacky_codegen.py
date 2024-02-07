from typing import List, Mapping


type_to_parsing_fn = {
    "i32": "parse_signed(stream)",
    "i64": "parse_signed(stream)",
    "f32": "parse_float32(stream)",
    "f64": "parse_float64(stream)",
}

INSTRUCTION_CLASS_TEMPLATE = """
@dataclass
class {class_name}(Instruction):
    {attributes}

    @classmethod
    def parse(cls, op: int, stream: Iterator[bytes]):
        return cls({constructor_args})

"""

WARNING = """\"\"\"
Do not edit, this is generated via scripts/hacky_codegen.py!
\"\"\"

"""

IMPORTS = """from dataclasses import dataclass
from typing import Iterator, Mapping, Tuple

from .instructions import Instruction
from .utils import parse_float32, parse_float64, parse_signed, parse_unsigned
from .wasm_types import i32, i64, f32, f64

"""

MAPPING_TEMPLATE = """
id_to_generated_instruction_class: Mapping[int, Instruction] = {{
    {mapping}
}}

"""


def gen(instructions: Mapping[str, List[str]]) -> str:
    """
    generate classes for <instructions>
    """
    args_template = "args: Tuple[{arg_types}]"
    name_template = 'name: str = "{name}"'

    mapping = []
    classes = ""

    for id, (name, args) in instructions.items():
        class_name = name.replace(".", "_")
        mapping.append(f"{hex(id)}: {class_name}")
        constructor_args = ""
        attributes = []
        if len(args) > 0:
            constructor_args = (
                f"({', '.join(map(lambda x: type_to_parsing_fn[x], args))})"
            )
            arg_types = ", ".join(args)
            attributes.append(args_template.format(arg_types=arg_types))
        attributes.append(name_template.format(name=name))
        classes += INSTRUCTION_CLASS_TEMPLATE.format(
            name=name,
            class_name=class_name,
            constructor_args=constructor_args,
            attributes="\n    ".join(attributes),
        )

    mapping_pairs = ",\n    ".join(mapping)

    return WARNING + IMPORTS + classes + MAPPING_TEMPLATE.format(mapping=mapping_pairs)


if __name__ == "__main__":
    with open("../generated_instructions.py", "w") as file:

        instructions_to_gen = {
            0x41: ("i32.const", ["i32"]),
            0x42: ("i64.const", ["i64"]),
            0x43: ("f32.const", ["f32"]),
            0x44: ("f64.const", ["f64"]),
            #
            0x45: ("i32.eqz", []),
            0x46: ("i32.eq", []),
            0x47: ("i32.ne", []),
            0x48: ("i32.lt_s", []),
            0x49: ("i32.lt_u", []),
            0x4A: ("i32.gt_s", []),
            0x4B: ("i32.gt_u", []),
            0x4C: ("i32.le_s", []),
            0x4D: ("i32.le_u", []),
            0x4E: ("i32.ge_s", []),
            0x4F: ("i32.ge_u", []),
            #
            0x50: ("i64.eqz", []),
            0x51: ("i64.eq", []),
            0x52: ("i64.ne", []),
            0x53: ("i64.lt_s", []),
            0x54: ("i64.lt_u", []),
            0x55: ("i64.gt_s", []),
            0x56: ("i64.gt_u", []),
            0x57: ("i64.le_s", []),
            0x58: ("i64.le_u", []),
            0x59: ("i64.ge_s", []),
            0x5A: ("i64.ge_u", []),
            #
        }

        file.write(gen(instructions_to_gen))
