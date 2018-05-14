import sys
import ujson


def type_detect(data):
    try:
        if 'pandas' in sys.modules:
            import pandas as pd
            if isinstance(data, pd.DataFrame):
                if isinstance(data.index, pd.DatetimeIndex):
                    df = data.reset_index()
                    df['index'] = df['index'].astype(str)
                    return 'pandas', df.to_json(orient='records')
                else:
                    return 'pandas', data.reset_index().to_json(orient='records')
    except ImportError:
        pass

    try:
        if 'lantern' in sys.modules:
            import lantern as l
            if isinstance(data, l.LanternLive):
                return 'lantern', data.path()
    except ImportError:
        pass

    try:
        if 'pyarrow' in sys.modules:
            import pyarrow as pa
            if isinstance(data, pa.Array) or isinstance(data, pa.Buffer):
                # TODO
                pass
    except ImportError:
        pass

    if isinstance(data, str):
        if ('http://' in data and 'http://' == data[:7]) or \
           ('https://' in data and 'https://' == data[:8]) or \
           ('ws://' in data and 'ws://' == data[:5]) or \
           ('wss://' in data and 'wss://' == data[:6]) or \
           ('sio://' in data and 'sio://' == data[:6]):
            return 'url', data

    elif isinstance(data, dict):
        return 'dict', ujson.dumps([data])

    elif isinstance(data, list):
        return 'list', ujson.dumps(data)

    # throw error?
    return '', data
