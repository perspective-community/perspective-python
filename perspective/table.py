import numpy as np
from .libbinding import t_schema, t_dtype, t_table


class Perspective(object):
    def __init__(self, column_names, types):
        self._columns = {}

        dtypes = []
        for name, _type in zip(column_names, types):
            dtypes.append(self._type_to_dtype(_type))
            self._columns[name] = _type

        if len(column_names) != len(dtypes):
            raise Exception('column name/dtype length mismatch')

        _schema = t_schema(column_names, dtypes)
        self._t_table = t_table(_schema)
        self._t_table.init()

    def _type_to_dtype(self, _type):
        if isinstance(_type, t_dtype):
            return _type
        if isinstance(_type, np.ndarray):
            if _type.dtype == np.int64:
                return t_dtype.NP_INT64
            if _type.dtype == np.float64:
                return t_dtype.NP_FLOAT64
            if _type.dtype == np.str:
                return t_dtype.NP_STR
            if _type.dtype == np.bool:
                return t_dtype.NP_BOOL
            if _type.dtype == np.complex128:
                return t_dtype.NP_COMPLEX128
        if _type == np.int64:
            return t_dtype.NP_INT64
        if _type == np.float64:
            return t_dtype.NP_FLOAT64
        if _type == np.str:
            return t_dtype.NP_STR
        if _type == np.bool:
            return t_dtype.NP_BOOL
        if _type == np.complex128:
            return t_dtype.NP_COMPLEX128
        if _type == int:
            return t_dtype.INT64
        if _type == float:
            return t_dtype.FLOAT64
        if _type == bool:
            return t_dtype.BOOL
        if _type == str:
            return t_dtype.STR
        else:
            raise Exception('%s not currently supported' % _type)

    def _validate_col(self, col):
        for i in range(len(col)):
            if i == 0:
                continue
            if not isinstance(col[i-1], type(col[i])) and not (isinstance(col[i], type(col[i-1]))):
                raise Exception('data must be homogenous type')

    def load(self, col, data):
        if self._t_table.size() < len(data):
            self._t_table.extend(len(data))

        if not self._t_table.get_schema().has_column(col):
            raise Exception('schema change not implemented')

        if isinstance(data, np.ndarray):
            self._t_table.load_column(col, data, self._type_to_dtype(data))
            return

        self._validate_col(data)
        self._t_table.load_column(col, data, self._type_to_dtype(type(data[0])))

    def print(self):
        self._t_table.pprint()

    def __repr__(self):
        # FIXME
        self.print()
        return ''
