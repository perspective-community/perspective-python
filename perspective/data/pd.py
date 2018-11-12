import sys
from .base import Data


class PandasData(Data):
    def __init__(self, data):
        import pandas as pd
        # level unstacking
        kwargs = {}

        if isinstance(data, pd.DataFrame) and isinstance(data.columns, pd.MultiIndex):
            data = pd.DataFrame(data.unstack())
            columns = list(x for x in data.index.names if x)
            kwargs['columnpivots'] = list(x for x in data.index.names if x)
            kwargs['rowpivots'] = []
            orig_columns = [' ' for _ in data.columns.tolist()]

            # deal with indexes
            if len(columns) < len(data.index.names):
                for i in range(len(data.index.names) - len(columns)):
                    if i == 0:
                        columns.append('index')
                        kwargs['rowpivots'].append('index')
                    else:
                        columns.append('index-{}'.format(i))
                        kwargs['rowpivots'].append('index-{}'.format(i))

            # all columns in index
            columns += orig_columns
            data.reset_index(inplace=True)
            data.columns = columns

            # use these columns
            kwargs['columns'] = orig_columns

        if isinstance(data.index, pd.MultiIndex):
            kwargs['rowpivots'] = list(data.index.names)
            kwargs['columns'] = data.columns

        # copy or not
        if isinstance(data, pd.Series) or 'index' not in map(lambda x: str(x).lower(), data.columns):
            df_processed = data.reset_index()
        else:
            df_processed = data

        # schema
        if isinstance(data, pd.Series):
            schema = {data.name: str(data.dtype)}
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
