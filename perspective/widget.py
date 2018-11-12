from ipywidgets import Widget
from six import iteritems
from traitlets import Unicode, List, Bool, Int, Dict, Any, validate
from .data import type_detect
from ._layout import validate_view, validate_columns, validate_rowpivots, validate_columnpivots, validate_aggregates, validate_sort
from ._schema import validate_schema


class PerspectiveWidget(Widget):
    _model_name = Unicode('PerspectiveModel').tag(sync=True)
    _model_module = Unicode('@jpmorganchase/perspective-jupyterlab').tag(sync=True)
    _model_module_version = Unicode('0.2.7').tag(sync=True)
    _view_name = Unicode('PerspectiveView').tag(sync=True)
    _view_module = Unicode('@jpmorganchase/perspective-jupyterlab').tag(sync=True)
    _view_module_version = Unicode('0.2.7').tag(sync=True)

    _data = List(default_value=[]).tag(sync=True)
    _dat_orig = Any()

    datasrc = Unicode(default_value='static').tag(sync=True)
    schema = Dict(default_value={}).tag(sync=True)
    view = Unicode('hypergrid').tag(sync=True)
    columns = List(default_value=[]).tag(sync=True)
    rowpivots = List(trait=Unicode(), default_value=[]).tag(sync=True, o=True)
    columnpivots = List(trait=Unicode(), default_value=[]).tag(sync=True)
    aggregates = Dict(trait=Unicode(), default_value={}).tag(sync=True)
    sort = List(default_value=[]).tag(sync=True)
    index = Unicode(default_value='').tag(sync=True)
    limit = Int(default_value=-1).tag(sync=True)

    settings = Bool(True).tag(sync=True)
    dark = Bool(False).tag(sync=True)

    def delete(self):
        self.send({'type': 'delete'})

    def update(self, data):
        dat_obj = type_detect(data)
        self.send({'type': 'update', 'data': dat_obj.data})

    def load(self, value):
        data_object = type_detect(value)

        # len in case dataframe
        if len(data_object.data) and data_object.type:
            s = validate_schema(data_object.schema)
            self.schema = s

            if not self.columns and 'columns' not in data_object.kwargs:
                columns = list(map(lambda x: str(x), s.keys()))

                # reasonable default, pivot by default in non-grid view
                if not self.rowpivots and self.view != 'hypergrid':
                    if 'index' in columns:
                        self.rowpivots = ['index']
                        if 'index' in columns:
                            columns.remove('index')
                self.columns = columns

            elif 'columns' in data_object.kwargs:
                columns = list(map(lambda x: str(x), data_object.kwargs.pop('columns')))
                self.columns = columns

        else:
            self.schema = {}

        for k, v in iteritems(data_object.kwargs):
            setattr(self, k, v)

        # set data last
        self._data = data_object.data

    @validate('datasrc')
    def _validate_datasrc(self, proposal):
        datasrc = proposal.value
        return datasrc

    @validate('schema')
    def _validate_schema(self, proposal):
        schema = proposal.value
        return schema

    @validate('view')
    def _validate_view(self, proposal):
        view = validate_view(proposal.value)
        return view

    @validate('columns')
    def _validate_columns(self, proposal):
        columns = validate_columns(proposal.value)
        return columns

    @validate('rowpivots')
    def _validate_rowpivots(self, proposal):
        rowpivots = validate_rowpivots(proposal.value)
        return rowpivots

    @validate('columnpivots')
    def _validate_columnpivots(self, proposal):
        columnpivots = validate_columnpivots(proposal.value)
        return columnpivots

    @validate('aggregates')
    def _validate_aggregates(self, proposal):
        aggregates = validate_aggregates(proposal.value)
        return aggregates

    @validate('sort')
    def _validate_sort(self, proposal):
        sort = validate_sort(proposal.value)
        return sort

    def __del__(self):
        self.send({'type': 'delete'})

    def __init__(self, data, view='hypergrid', schema=None, columns=None, rowpivots=None, columnpivots=None, aggregates=None, sort=None, index='', limit=-1, settings=True, dark=False, *args, **kwargs):
        super(PerspectiveWidget, self).__init__(**kwargs)
        self._helper = None
        self.datasrc = 'static'
        self.view = validate_view(view)
        self.schema = schema or {}
        self.sort = validate_sort(sort) or []
        self.index = index
        self.limit = limit
        self.settings = settings
        self.dark = dark

        self.rowpivots = validate_rowpivots(rowpivots) or []
        self.columnpivots = validate_columnpivots(columnpivots) or []
        self.aggregates = validate_aggregates(aggregates) or {}

        self.columns = validate_columns(columns) or []
        self.load(data)
