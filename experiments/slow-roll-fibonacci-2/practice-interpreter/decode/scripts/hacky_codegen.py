from typing import List, Mapping


type_to_parsing_fn = {
    "i32": "parse_signed(stream)",
    "i64": "parse_signed(stream)",
    "f32": "parse_float32(stream)",
    "f64": "parse_float64(stream)",
    "local_idx": "parse_signed(stream)",
    "global_idx": "parse_signed(stream)",
    "label_idx": "parse_signed(stream)",
}

INSTRUCTION_CLASS_TEMPLATE = """
@dataclass
class {class_name}(Instruction):
    {attributes}

    def accept(self, visitor: "Visitor") -> None:
        visitor.visit_{class_name}(self)

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls({constructor_args})

"""

VISITOR_CLASS_TEMPLATE = """
class GeneratedVisitor(ABC):
{visitor_methods}

"""

VISITOR_METHOD_TEMPLATE = """
    @abstractmethod
    def visit_{class_name}(self, instruction: {class_name}) -> None:
        pass
"""

MAPPING_TEMPLATE = """
def op_code_to_generated_class() -> Dict[int, Instruction]:
    return {{
        {mapping}
    }}

"""


def gen(instructions: Mapping[str, List[str]]) -> str:
    """
    generate classes for <instructions>
    """
    args_template = "args: Tuple[{arg_types}]"
    name_template = 'name: str = "{name}"'

    visitor_methods = ""
    mapping = []
    classes = ""

    for id, (name, args) in instructions.items():
        class_name = name.replace(".", "_")
        mapping.append(f"{hex(id)}: {class_name}")
        constructor_args = ""
        attributes = []
        if len(args) > 0:
            constructor_args = (
                # The extra comma in the string below ensures that python
                # evaluates the arguments as a tuple. Python  evaluates `(1)` to
                # `1`, but it evaluates `(1,)` to a tuple containing `1`. We
                # want the latter.
                f"({', '.join(map(lambda x: type_to_parsing_fn[x], args))},)"
            )
            arg_types = ", ".join(args)
            attributes.append(args_template.format(arg_types=arg_types))
        attributes.append(name_template.format(name=name))
        visitor_methods += VISITOR_METHOD_TEMPLATE.format(class_name=class_name)
        classes += INSTRUCTION_CLASS_TEMPLATE.format(
            name=name,
            class_name=class_name,
            constructor_args=constructor_args,
            attributes="\n    ".join(attributes),
        )

    mapping_pairs = (",\n" + " " * 8).join(mapping)

    return (
        classes
        + MAPPING_TEMPLATE.format(mapping=mapping_pairs)
        + VISITOR_CLASS_TEMPLATE.format(visitor_methods=visitor_methods)
    )


if __name__ == "__main__":
    instructions_to_gen = {
        0x00: ("unreachable", []),
        0x01: ("nop", []),
        0x0C: ("br", ["label_idx"]),  #
        0x0D: ("br_if", ["label_idx"]),  #
        # ===
        0x20: ("local.get", ["local_idx"]),  #
        0x21: ("local.set", ["local_idx"]),  #
        0x22: ("local.tee", ["local_idx"]),  #
        # 0x23: ("global.get", ["global_idx"]),
        # 0x24: ("global.set", ["global_idx"]),
        # ===
        0x41: ("i32.const", ["i32"]),  #
        # 0x42: ("i64.const", ["i64"]),
        # 0x43: ("f32.const", ["f32"]),
        # 0x44: ("f64.const", ["f64"]),
        # ===
        # 0x45: ("i32.eqz", []),
        # 0x46: ("i32.eq", []),
        # 0x47: ("i32.ne", []),
        # 0x48: ("i32.lt_s", []),
        # 0x49: ("i32.lt_u", []),
        # 0x4A: ("i32.gt_s", []),
        # 0x4B: ("i32.gt_u", []),
        # 0x4C: ("i32.le_s", []),
        # 0x4D: ("i32.le_u", []),
        0x4E: ("i32.ge_s", []),  #
        # 0x4F: ("i32.ge_u", []),
        # ===
        # 0x50: ("i64.eqz", []),
        # 0x51: ("i64.eq", []),
        # 0x52: ("i64.ne", []),
        # 0x53: ("i64.lt_s", []),
        # 0x54: ("i64.lt_u", []),
        # 0x55: ("i64.gt_s", []),
        # 0x56: ("i64.gt_u", []),
        # 0x57: ("i64.le_s", []),
        # 0x58: ("i64.le_u", []),
        # 0x59: ("i64.ge_s", []),
        # 0x5A: ("i64.ge_u", []),
        # ===
        # 0x5B: ("f32.eq", []),
        # 0x5C: ("f32.ne", []),
        # 0x5D: ("f32.lt", []),
        # 0x5E: ("f32.gt", []),
        # 0x5F: ("f32.le", []),
        # 0x60: ("f32.ge", []),
        # ===
        # 0x61: ("f64.eq", []),
        # 0x62: ("f64.ne", []),
        # 0x63: ("f64.lt", []),
        # 0x64: ("f64.gt", []),
        # 0x65: ("f64.le", []),
        # 0x66: ("f64.ge", []),
        # ===
        # 0x67: ("i32.clz", []),
        # 0x68: ("i32.ctz", []),
        # 0x69: ("i32.popcnt", []),
        0x6A: ("i32.add", []),
        # 0x6B: ("i32.sub", []),
        # 0x6C: ("i32.mul", []),
        # 0x6D: ("i32.div_s", []),
        # 0x6E: ("i32.div_u", []),
        # 0x6F: ("i32.rem_s", []),
        # 0x70: ("i32.rem_u", []),
        # 0x71: ("i32.and", []),
        # 0x72: ("i32.or", []),
        # 0x73: ("i32.xor", []),
        # 0x74: ("i32.shl", []),
        # 0x75: ("i32.shr_s", []),
        # 0x76: ("i32.shr_u", []),
        # 0x77: ("i32.rtol", []),
        # 0x78: ("i32.rtor", []),
    }

    start_marker = "##GEN_START##"
    stop_marker = "##GEN_STOP##"
    replacement_content = ""
    with open("../instructions.py", "r") as file:
        lines = file.readlines()

        between_start_and_stop_marker = False
        for line in lines:
            if start_marker in line:
                between_start_and_stop_marker = True
                replacement_content += line  # Keep "##GEN_START"
                replacement_content += gen(instructions_to_gen)
            if stop_marker in line:
                between_start_and_stop_marker = False
                replacement_content += line  # Keep "##GEN_STOP"
            elif between_start_and_stop_marker:
                continue
            else:
                replacement_content += line

        with open("../instructions.py", "w") as file:
            file.write(replacement_content)
