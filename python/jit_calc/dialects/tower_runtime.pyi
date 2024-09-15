from typing import ClassVar, Optional

from ..ir import Type, TypeID, Context
from ._tower_runtime_ops_gen import *

class ContextType(Type):
    static_typeid: ClassVar[TypeID]

    @staticmethod
    def get(context: Optional[Context] = None) -> "ContextType":
        """
        Create a !tower_runtime.context type.
        """

    @staticmethod
    def isinstance(other: Type) -> bool: ...
    def __init__(self, cast_from_type: Type) -> None: ...
    @property
    def typeid(self) -> TypeID: ...
