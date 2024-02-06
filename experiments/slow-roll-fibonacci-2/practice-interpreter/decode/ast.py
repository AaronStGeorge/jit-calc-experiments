from abc import ABC, abstractmethod
from typing import Iterator


class AST(ABC):
    """
    Base class for wasm decode AST
    """

    @classmethod
    @abstractmethod
    def parse(cls, stream: Iterator[bytes]) -> "AST":
        """
        Factory method to hydrate class
        """
