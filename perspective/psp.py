from enum import Enum


def psp(data, view='hypergrid', columns=None, rowpivots=None, columnpivots=None, aggregates=None, settings=False):
    '''Render a perspective javascript widget in jupyter

    [description]

    Arguments:
        data {dataframe or live source} -- The static or live datasource

    Keyword Arguments:
        view {str/View} -- what view to use. available in the enum View (default: {'hypergrid'})
        columns {[string]} -- what columns to display
        rowpivots {[string]} -- what names to use as rowpivots
        columnpivots {[string]} -- what names to use as columnpivots
        aggregates {dict(str, str/Aggregate)} -- dictionary of name to aggregate type (either string or enum Aggregate)
        settings {bool} -- display settings
    '''
    from IPython.display import display
    bundle = {}
    bundle['application/psp'] = {
        'data': _type_detect(data),
        'layout': _layout(view, columns, rowpivots, columnpivots, aggregates, settings)
    }
    display(bundle, raw=True)


def _layout(view='hypergrid', columns=None, rowpivots=None, columnpivots=None, aggregates=None, settings=False):
    import ujson
    ret = {}

    if isinstance(view, View):
        ret['view'] = view.value
    elif isinstance(view, str):
        if view not in View.options():
            raise Exception('Unrecognized view: %s', view)
        ret['view'] = view
    else:
        raise Exception('Cannot parse view type: %s', str(type(view)))

    if columns is None:
        ret['columns'] = ''
    elif isinstance(columns, list):
        ret['columns'] = columns
    else:
        raise Exception('Cannot parse columns type: %s', str(type(columns)))

    if rowpivots is None:
        ret['row-pivots'] = ''
    elif isinstance(rowpivots, list):
        ret['row-pivots'] = rowpivots
    else:
        raise Exception('Cannot parse rowpivots type: %s', str(type(rowpivots)))

    if columnpivots is None:
        ret['columnpivots'] = ''
    elif isinstance(columnpivots, list):
        ret['columnpivots'] = columnpivots
    else:
        raise Exception('Cannot parse columnpivots type: %s', str(type(columnpivots)))

    if aggregates is None:
        ret['aggregates'] = ''
    elif isinstance(aggregates, dict):
        for k, v in aggregates.items():
            if isinstance(v, Aggregate):
                aggregates[k] = v.value
            elif isinstance(v, str):
                if v not in Aggregate.options():
                    raise Exception('Unrecognized aggregate: %s', v)
            else:
                raise Exception('Cannot parse aggregation of type %s', str(type(v)))
        ret['aggregates'] = aggregates
    else:
        raise Exception('Cannot parse aggregates type: %s', str(type(aggregates)))

    return ujson.dumps(ret)


def _type_detect(data):
    try:
        import pandas as pd
        if isinstance(data, pd.DataFrame):
            return data.reset_index().to_json(orient='records')
    except ImportError:
        pass

    try:
        import lantern as l
        if isinstance(data, l.LanternLive):
            return data.path()
    except ImportError:
        pass
    return data


class Aggregate(Enum):
    ANY = 'any'
    AVG = 'avg'
    COUNT = 'count'
    DISTINCT_COUNT = 'distinct_count'
    DOMINANT = 'dominant'
    FIRST = 'first'
    LAST = 'last'
    HIGH = 'high'
    LOW = 'low'
    MEAN = 'mean'
    MEAN_BY_COUNT = 'mean_by_count'
    MEDIAN = 'median'
    PCT_SUM_PARENT = 'pct_sum_parent'
    PCT_SUM_GRAND_TOTAL = 'pct_sum_grand_total'
    SUM = 'sum'
    SUM_ABS = 'sum_abs'
    SUM_NOT_NULL = 'sum_not_null'
    UNIQUE = 'unique'

    @staticmethod
    def options():
        return list(map(lambda c: c.value, Aggregate))


class View(Enum):
    HYPERGRID = 'hypergrid'
    VERTICAL = 'vertical'
    HORIZONTAL = 'horizontal'
    LINE = 'line'
    SCATTER = 'scatter'

    @staticmethod
    def options():
        return list(map(lambda c: c.value, View))
