def psp(data):
    from IPython.display import display
    bundle = {}
    bundle['application/psp'] = _type_detect(data)
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
