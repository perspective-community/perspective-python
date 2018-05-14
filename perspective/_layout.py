import ujson
from six import iteritems
from .exception import PSPException
from .view import View
from .aggregate import Aggregate


def layout(view='hypergrid', columns=None, rowpivots=None, columnpivots=None, aggregates=None, sort=None, settings=False, dark=False):
    ret = {}

    if isinstance(view, View):
        ret['view'] = view.value
    elif isinstance(view, str):
        if view not in View.options():
            raise PSPException('Unrecognized view: %s', view)
        ret['view'] = view
    else:
        raise PSPException('Cannot parse view type: %s', str(type(view)))

    if columns is None:
        ret['columns'] = ''
    elif isinstance(columns, str):
        ret['columns'] = [columns]
    elif isinstance(columns, list):
        ret['columns'] = columns
    else:
        raise PSPException('Cannot parse columns type: %s', str(type(columns)))

    if rowpivots is None:
        ret['row-pivots'] = ''
    elif isinstance(rowpivots, str):
        ret['row-pivots'] = [rowpivots]
    elif isinstance(rowpivots, list):
        ret['row-pivots'] = rowpivots
    else:
        raise PSPException('Cannot parse rowpivots type: %s', str(type(rowpivots)))

    if columnpivots is None:
        ret['column-pivots'] = ''
    elif isinstance(columnpivots, str):
        ret['column-pivots'] = [columnpivots]
    elif isinstance(columnpivots, list):
        ret['column-pivots'] = columnpivots
    else:
        raise PSPException('Cannot parse columnpivots type: %s', str(type(columnpivots)))

    if aggregates is None:
        ret['aggregates'] = ''
    elif isinstance(aggregates, dict):
        for k, v in iteritems(aggregates):
            if isinstance(v, Aggregate):
                aggregates[k] = v.value
            elif isinstance(v, str):
                if v not in Aggregate.options():
                    raise PSPException('Unrecognized aggregate: %s', v)
            else:
                raise PSPException('Cannot parse aggregation of type %s', str(type(v)))
        ret['aggregates'] = aggregates
    else:
        raise PSPException('Cannot parse aggregates type: %s', str(type(aggregates)))

    if sort is None:
        ret['sort'] = ''
    elif isinstance(sort, str):
        ret['sort'] = [sort]
    elif isinstance(sort, list):
        ret['sort'] = sort
    else:
        raise PSPException('Cannot parse sort type: %s', str(type(sort)))

    ret['settings'] = settings

    if dark:
        ret['colorscheme'] = 'dark'

    return ujson.dumps(ret)
