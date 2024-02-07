from typing import Iterator, Mapping
from abc import abstractmethod

from .ast import AST


def get_id_to_instruction_class():
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

    return id_to_generated_instruction_class


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
