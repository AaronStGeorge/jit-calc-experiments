from typing import Iterator, Mapping
from abc import abstractmethod

from .ast import AST

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
    @abstractmethod
    def parse(cls, stream: Iterator[bytes]):
        from .generated_instructions import id_to_generated_instruction_class
        op_code_to_class: Mapping[int, Instruction] = id_to_generated_instruction_class
        b = next(stream)
        instr = op_code_to_class[b].parse(stream)
        return instr
