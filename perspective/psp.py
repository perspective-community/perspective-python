from enum import Enum
try:
    from html import unescape  # python 3.4+
except ImportError:
    try:
        from html.parser import HTMLParser  # python 3.x (<3.4)
    except ImportError:
        from HTMLParser import HTMLParser  # python 2.x
    unescape = HTMLParser().unescape


def psp(data, view='hypergrid', columns=None, rowpivots=None, columnpivots=None, aggregates=None, settings=False):
    from IPython.display import display
    bundle = {}
    bundle['application/psp'] = {
        'data': _type_detect(data),
        'layout': _layout(view, columns, rowpivots, columnpivots, aggregates, settings)
    }
    display(bundle, raw=True)


def _layout(view='hypergrid', columns=None, rowpivots=None, columnpivots=None, aggregates=None, settings=False):
    import ujson
    '''
        view="hypergrid"  # grid
        view="horizontal"  # horizontal
        view="vertical"  # vertical
        view="line"  # line
        view="scatter"  # scatter
    '''
    ret = {}

    ret['view'] = view.value if isinstance(view, View) else view

    ret['columns'] = columns or ''
    ret['row-pivots'] = rowpivots or ''
    ret['column-pivots'] = columnpivots or ''
    ret['aggregates'] = aggregates or ''
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


class Aggregates(Enum):
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


class View(Enum):
    HYPERGRID = 'hypergrid'
    VERTICAL = 'vertical'
    HORIZONTAL = 'horizontal'
    LINE = 'line'
    SCATTER = 'scatter'
