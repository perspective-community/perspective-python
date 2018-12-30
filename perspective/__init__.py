from .base import PerspectiveBaseMixin  # noqa: F401
from .psp import psp
from .view import View  # noqa: F401
from .aggregate import Aggregate  # noqa: F401
from .exception import PSPException  # noqa: F401
from .widget import PerspectiveWidget  # noqa: F401


# export `plot` and `table` functions
table = psp
plot = psp

__version__ = '0.1.3'
