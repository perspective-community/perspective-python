from .base import _is_dict, _is_list, Data
from .pd import _is_pandas

EXPORTERS = [_is_dict, _is_list, _is_pandas]


def type_detect(data):
    for foo in EXPORTERS:
        data_object = foo(data)
        if data_object.type:
            return data_object
    # throw error?
    return Data.Empty()
