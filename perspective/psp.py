from functools import wraps
from .widget import PerspectiveWidget


def _or_default(name, _def):
    return name if name else _def


@wraps(PerspectiveWidget.__init__)
def psp(data=None,
        view='hypergrid',
        schema=None,
        columns=None,
        rowpivots=None,
        columnpivots=None,
        aggregates=None,
        sort=None,
        index='',
        limit=-1,
        computedcolumns=None,
        filters=None,
        plugin_config=None,
        settings=True,
        embed=False,
        dark=False,
        helper_config=None):
    data = [] if data is None else data
    return PerspectiveWidget(data=data,
                             view=view,
                             schema=schema,
                             columns=columns,
                             rowpivots=rowpivots,
                             columnpivots=columnpivots,
                             aggregates=aggregates,
                             sort=sort,
                             index=index,
                             limit=limit,
                             computedcolumns=computedcolumns,
                             filters=filters,
                             plugin_config=plugin_config,
                             settings=settings,
                             embed=embed,
                             dark=dark)
