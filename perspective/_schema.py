from six import iteritems
import ujson


def convert_to_psp_schema(schema):
    d = {}
    for k, v in iteritems(schema):
        if 'float' in v:
            d[k] = 'float'
        elif 'int' in v:
            d[k] = 'int'
        elif 'bool' in v:
            d[k] = 'boolean'
        elif ':' in v or '-' in v or 'date' in v or 'time' in v:
            d[k] = 'date'
        elif 'str' in v or 'string' in v:
            d[k] = 'string'
    return d


def schema(data, typ):
    if typ in ('', 'url', 'lantern', 'pyarrow'):
        # TODO
        return ''
    elif typ == 'dict':
        schema = {k: str(type(v)) for k, v in iteritems(data)}
    elif typ == 'list':
        if isinstance(data[0], dict):
            schema = {k: str(type(v)) for k, v in iteritems(data[0])}
        else:
            # TODO
            raise NotImplemented()
    elif typ == 'pandas':
        schema = dict(data.reset_index().dtypes.astype(str))
    else:
        raise NotImplemented()

    schema = convert_to_psp_schema(schema)
    return ujson.dumps(schema)
