
BOOLEAN_FILTERS = ["&", "|", "==", "!=", "or", "and"]
NUMBER_FILTERS = ["<", ">", "==", "<=", ">=", "!=", "is nan", "is not nan"]
STRING_FILTERS = ["==", "contains", "!=", "in", "not in", "begins with", "ends with"]
DATETIME_FILTERS = ["<", ">", "==", "<=", ">=", "!="]

ALL_FILTERS = BOOLEAN_FILTERS + NUMBER_FILTERS + STRING_FILTERS + DATETIME_FILTERS
