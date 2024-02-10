from dataclasses import dataclass
from typing import Iterator, List, Mapping, Tuple
from abc import abstractmethod

from .ast import AST
from .utils import parse_valtype


class Instruction(AST):
    """
    https://webassembly.github.io/spec/core/binary/instructions.html#instructions
    """

    @property
    @abstractmethod
    def name(self) -> str:
        """
        Name of instruction
        """

    @classmethod
    def parse(cls, op_code: int, stream: Iterator[bytes]):
        op_code_to_class: Mapping[int, Instruction] = get_id_to_instruction_class()
        return op_code_to_class[op_code].parse(stream)


def parse_instruction_list(stream: Iterator[bytes]) -> List[Instruction]:
    """
    Parse instructions until op_code 0x0B comes up. Wasm spec uses this in
    several places.
    """
    instructions: List[Instruction] = []
    while True:
        op_code = next(stream)
        if op_code == 0x0B:
            break
        instructions.append(Instruction.parse(op_code, stream))
    return instructions


def get_id_to_instruction_class() -> Mapping[int, Instruction]:
    """
    This is a hack to get around circular dependency between Instruction
    (defined in this module) and the generated instruction classes that inherit
    from Instruction. The generated instructions should really be in this file
    but then it's difficult to update, we would have to search for anything
    between two comments or something to dump updated generated classes. We
    could probably use `eval` too, but we all know what happens when someone
    decides to get all "dynamic" up in here: https://youtu.be/wrZ7PnolbQ4?t=23 ;
    and that just wouldn't be fun.
    """
    from .generated_instructions import id_to_generated_instruction_class

    id_to_instruction_class: Mapping[int, Instruction] = {
        0x02: block,
        0x03: loop,
    }

    return id_to_generated_instruction_class | id_to_instruction_class


@dataclass
class block(Instruction):
    block_type: str
    instructions: List[Instruction]
    name: str = "block"

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls(parse_valtype(stream), parse_instruction_list(stream))


@dataclass
class loop(Instruction):
    block_type: str
    instructions: List[Instruction]
    name: str = "loop"

    @classmethod
    def parse(cls, stream: Iterator[bytes]):
        return cls(parse_valtype(stream), parse_instruction_list(stream))
