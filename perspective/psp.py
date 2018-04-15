import sys
import ujson
from enum import Enum


class PSPException(Exception):
    pass


def psp(data, view='hypergrid', columns=None, rowpivots=None, columnpivots=None, aggregates=None, sort=None, settings=False, dark=False, helper_config=None):
    '''Render a perspective javascript widget in jupyter

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
    bundle['application/psp+json'] = {
        'data': _type_detect(data),
        'layout': _layout(view, columns, rowpivots, columnpivots, aggregates, sort, settings, dark),
        'config': _config(helper_config, data)
    }
    return display(bundle, raw=True)


def _layout(view='hypergrid', columns=None, rowpivots=None, columnpivots=None, aggregates=None, sort=None, settings=False, dark=False):
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
        for k, v in aggregates.items():
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


def _type_detect(data):
    try:
        if 'pandas' in sys.modules:
            import pandas as pd
            if isinstance(data, pd.DataFrame):
                if isinstance(data.index, pd.DatetimeIndex):
                    df = data.reset_index()
                    df['index'] = df['index'].astype(str)
                    return df.to_json(orient='records')
                else:
                    return data.reset_index().to_json(orient='records')
    except ImportError:
        pass

    try:
        if 'lantern' in sys.modules:
            import lantern as l
            if isinstance(data, l.LanternLive):
                return data.path()
    except ImportError:
        pass

    try:
        if 'pyarrow' in sys.modules:
            import pyarrow as pa
            if isinstance(data, pa.Array) or isinstance(data, pa.Buffer):
                pass
    except ImportError:
        pass

    if isinstance(data, str):
        if ('http://' in data and 'http://' == data[:7]) or \
           ('https://' in data and 'https://' == data[:8]) or \
           ('ws://' in data and 'ws://' == data[:5]) or \
           ('wss://' in data and 'wss://' == data[:6]) or \
           ('sio://' in data and 'sio://' == data[:6]):
            return data

    elif isinstance(data, dict):
        return ujson.dumps([data])

    elif isinstance(data, list):
        return ujson.dumps(data)

    # throw error?
    return data


def _config(config, data):
    if not isinstance(data, str):
        return ''

    config = {} if config is None else config
    if isinstance(config, str):
        config = ujson.loads(config)

    if ('http://' in data and 'http://' == data[:7]) or \
       ('https://' in data and 'https://' == data[:8]):

        ret = {}
        ret['field'] = config.get('field', '')
        ret['records'] = config.get('records', False)
        ret['repeat'] = config.get('repeat', 10)
        return ujson.dumps(ret)

    elif ('ws://' in data and 'ws://' == data[:5]) or \
         ('wss://' in data and 'wss://' == data[:6]):

        ret = {}
        ret['send'] = ujson.dumps(config.get('send', {}))  # double json
        ret['records'] = config.get('records', False)
        return ujson.dumps(ret)

    elif ('sio://' in data and 'sio://' == data[:6]):

        ret = {}
        ret['channel'] = config.get('channel', '')
        ret['records'] = config.get('records', False)
        return ujson.dumps(ret)

    else:
        return ''


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
    GRID = 'hypergrid'
    YBAR = 'y_bar'
    XBAR = 'x_bar'
    YLINE = 'y_line'
    YAREA = 'y_area'
    XYLINE = 'xy_line'
    XYSCATTER = 'xy_scatter'
    TREEMAP = 'treemap'
    HEATMAP = 'heatmap'

    @staticmethod
    def options():
        return list(map(lambda c: c.value, View))
