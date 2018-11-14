import json
from six import iteritems, string_types
from .computed import Functions
from .exception import PSPException
from .view import View
from .aggregate import Aggregate
from .sort import Sort


def validate_view(view):
    if isinstance(view, View):
        return view.value
    elif isinstance(view, string_types):
        if view not in View.options():
            raise PSPException('Unrecognized view: %s', view)
        return view
    else:
        raise PSPException('Cannot parse view type: %s', str(type(view)))


def validate_columns(columns):
    if columns is None:
        return []
    elif isinstance(columns, string_types):
        columns = [columns]

    if isinstance(columns, list):
        return columns
    else:
        raise PSPException('Cannot parse columns type: %s', str(type(columns)))


def _validate_pivots(pivots):
    if pivots is None:
        return []
    elif isinstance(pivots, string_types):
        pivots = [pivots]

    if isinstance(pivots, list):
        return pivots
    else:
        raise PSPException('Cannot parse rowpivots type: %s', str(type(pivots)))


def validate_rowpivots(rowpivots):
    return _validate_pivots(rowpivots)


def validate_columnpivots(columnpivots):
    return _validate_pivots(columnpivots)


def validate_aggregates(aggregates):
    if aggregates is None:
        return []
    elif isinstance(aggregates, dict):
        for k, v in iteritems(aggregates):
            if isinstance(v, Aggregate):
                aggregates[k] = v.value
            elif isinstance(v, string_types):
                if v not in Aggregate.options():
                    raise PSPException('Unrecognized aggregate: %s', v)
            else:
                raise PSPException('Cannot parse aggregation of type %s', str(type(v)))
        return aggregates
    else:
        raise PSPException('Cannot parse aggregates type: %s', str(type(aggregates)))


def validate_sort(sort):
    if sort is None:
        return []
    elif isinstance(sort, string_types):
        sort = [sort]

    if isinstance(sort, list):
        ret = []
        for col, s in sort:
            if isinstance(s, Sort):
                s = s.value
            elif not isinstance(s, string_types) or s not in Sort.options():
                raise PSPException('Unrecognized Sort: %s', s)
            ret.append([col, s])
        return ret
    else:
        raise PSPException('Cannot parse sort type: %s', str(type(sort)))


def validate_computedcolumns(computedcolumns, columns=[]):
    if computedcolumns is None:
        return []

    elif isinstance(computedcolumns, dict):
        computedcolumns = [computedcolumns]

    if isinstance(computedcolumns, list):
        ret = []
        for i, d in enumerate(computedcolumns):
            if not isinstance(d, dict):
                raise PSPException('Cannot parse computedcolumns')
            if 'inputs' not in d or 'func' not in d:
                raise PSPException('Cannot parse computedcolumns - inputs or func missing')
            if 'name' not in d:
                d['name'] = 'Anon-{}'.format(i)

            inputs = d['inputs']
            if not isinstance(inputs, list):
                raise PSPException('Cannot parse computedcolumns - inputs is not list')
            for i, input in enumerate(inputs):
                # FIXME check if column exists?
                if columns and input not in columns:
                    raise PSPException('Cannot parse computedcolumns - unrecognized column {}'.format(input))
                if not isinstance(input, string_types):
                    inputs[i] = str(input)

            if d['func'] not in Functions.options():
                raise PSPException('Cannot parse computedcolumns - unrecognized function {}'.format(d['func']))
            ret.append(d)
        return ret

    else:
        raise PSPException('Cannot parse computedcolumns type: %s', str(type(computedcolumns)))
