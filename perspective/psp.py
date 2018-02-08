import ujson


def psp(data, layout=None):
    from IPython.display import display
    bundle = {}
    bundle['application/psp'] = {
        'data': _type_detect(data),
        'layout': ujson.dumps(layout or [])
    }
    display(bundle, raw=True)


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
