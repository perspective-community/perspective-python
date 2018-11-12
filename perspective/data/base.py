from six import iteritems


class Data(object):
    def __init__(self, type, data, schema, **kwargs):
        self.type = type
        self.data = data
        self.schema = schema
        self.kwargs = kwargs

    @staticmethod
    def Empty():
        return Data('', [], {})


class DictData(Data):
    def __init__(self, data):
        super(DictData, self).__init__('dict', [data], {k: str(type(v)) for k, v in iteritems(data)})


class ListData(Data):
    def __init__(self, data):
        if len(data) > 0 and not isinstance(data[0], dict):
            raise NotImplementedError()
        super(ListData, self).__init__('list', data, {k: str(type(v)) for k, v in iteritems(data[0])} if len(data) > 0 else {})


def _is_dict(data, as_string=False):
    if isinstance(data, dict):
        return DictData(data)
    return Data.Empty()


def _is_list(data, as_string=False):
    if isinstance(data, list):
        return ListData(data)
    return Data.Empty()
