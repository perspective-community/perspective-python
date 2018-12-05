from .base import PerspectiveBaseMixin
from .psp import psp
from .view import View
from .aggregate import Aggregate
from .exception import PSPException
from .widget import PerspectiveWidget


# export `plot` and `table` functions
table = psp
plot = psp

__version__ = '0.1.2'
