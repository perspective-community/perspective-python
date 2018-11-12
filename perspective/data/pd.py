import sys
from .base import Data


class PandasData(Data):
    def __init__(self, data):
        import pandas as pd
        # level unstacking
        kwargs = {}
        if isinstance(data.index, pd.MultiIndex):
            kwargs['rowpivots'] = list(data.index.names)

        # copy or not
        if isinstance(data, pd.Series) or 'index' not in map(lambda x: str(x).lower(), data.columns):
            df_processed = data.reset_index()
        else:
            df_processed = data

        # schema
        if isinstance(data, pd.Series):
            schema = {data.name, str(data.dtype)}
        else:
            schema = df_processed.dtypes.astype(str)

        # datatype conversion
        for x in df_processed.dtypes.iteritems():
            if 'date' in str(x[1]):
                df_processed[x[0]] = df_processed[x[0]].astype(str)

        df_processed = df_processed.to_dict(orient='records')

        super(PandasData, self).__init__('pandas', df_processed, schema, **kwargs)


def _is_pandas(data):
    try:
        if 'pandas' in sys.modules:
            import pandas as pd
            if isinstance(data, pd.DataFrame) or isinstance(data, pd.Series):
                return PandasData(data)
    except ImportError:
        return Data.Empty()
    return Data.Empty()
