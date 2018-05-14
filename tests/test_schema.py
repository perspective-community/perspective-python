import pandas as pd


class TestLayout:
    def setup(self):
        pass
        # setup() before each test method

    def test_schema(self):
        from perspective._schema import schema
        assert schema(None, '') == '{}'
        assert schema(None, 'url') == '{}'
        assert schema(None, 'lantern') == '{}'
        assert schema(None, 'pyarrow') == '{}'

        x = schema({'test': 3}, 'dict')
        assert x == '{"test":"int"}'
        x = schema([{'test': 3}], 'list')
        assert x == '{"test":"int"}'

        x = pd.DataFrame([{'test': 3}])
        x = schema(x, 'pandas')
        assert x == '{"index":"int","test":"int"}'

    def test_convert_to_psp_schema(self):
        from perspective._schema import convert_to_psp_schema
        x = {1: 'float', 2: 'int', 3: 'bool', 4: 'time', 5: 'str'}
        y = {1: 'float', 2: 'int', 3: 'boolean', 4: 'date', 5: 'string'}
        assert y == convert_to_psp_schema(x)
