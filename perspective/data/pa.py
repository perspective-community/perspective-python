import sys
from .base import Data


class ArrowData(Data):
    def __init__(self, data):
        super(ArrowData, self).__init__('pyarrow', data.to_pybytes(), {})


def _is_pyarrow(data):
    try:
        if 'pyarrow' in sys.modules:
            import pyarrow as pa
            if isinstance(data, pa.lib.Buffer):
                return ArrowData(data)
    except ImportError:
        return Data.Empty()
    return Data.Empty()
