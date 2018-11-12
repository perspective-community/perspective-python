from .widget import PerspectiveWidget


def _or_default(name, _def):
    return name if name else _def


def psp(data=None,
        view='hypergrid',
        schema=None,
        columns=None,
        rowpivots=None,
        columnpivots=None,
        aggregates=None,
        sort=None,
        settings=True,
        dark=False,
        helper_config=None):
    '''Render a perspective javascript widget in jupyter

    Arguments:
        data : dataframe/list/dict
            The static or live datasource

    Keyword Arguments:
        view : str or View
            what view to use. available in the enum View (default: {'hypergrid'})
        columns : list of str
            what columns to display
        rowpivots : list of str
            what names to use as rowpivots
        columnpivots : list of str
            what names to use as columnpivots
        aggregates dict(str: str or Aggregate)
            dictionary of name to aggregate type (either string or enum Aggregate)
        settings : bool
            display settings
    '''
    data = [] if data is None else data
    p = PerspectiveWidget(data, view, schema, columns, rowpivots, columnpivots, aggregates, sort, settings, dark, helper_config)
    return p
