from enum import Enum


class Sort(Enum):
    ASC = 'asc'
    ASC_ABS = 'asc abs'
    COL_ASC = 'col asc'
    COL_ASC_ABS = 'col asc abs'
    COL_DESC = 'col desc'
    COL_DESC_ABS = 'col desc abs'
    DESC = 'desc'
    DESC_ABS = 'desc abs'
    NONE = 'none'

    @staticmethod
    def options():
        return list(map(lambda c: c.value, Sort))
