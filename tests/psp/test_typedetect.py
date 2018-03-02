from mock import patch, MagicMock
from datetime import datetime


class Nope(object):
    @property
    def DataFrame(self):
        raise ImportError

    @property
    def LanternLive(self):
        raise ImportError

    @property
    def Array(self):
        raise ImportError

    @property
    def Buffer(self):
        raise ImportError


class TestTypedetect:
    def setup(self):
        pass
        # setup() before each test method

    def teardown(self):
        pass
        # teardown() after each test method

    @classmethod
    def setup_class(cls):
        pass
        # setup_class() before any methods in this class

    @classmethod
    def teardown_class(cls):
        pass
        # teardown_class() after any methods in this class

    def test_pandas(self):
        import pandas as pd
        import ujson
        from perspective.psp import _type_detect

        df = pd.DataFrame([1, 2])
        x = _type_detect(df)

        expected = ujson.dumps([{"index": 0, "0": 1}, {"index": 1, "0": 2}])
        assert x == expected

        df = pd.DataFrame([[1, 2]], columns=['1', '2'], index=[datetime.today(), datetime.today()])
        x = _type_detect(df)

        import sys
        sys.modules['pandas'] = Nope()
        _type_detect('test')
        sys.modules['pandas'] = pd

    def test_lantern(self):
        class Test(object):
            def __init__(self):
                pass

            def path(self):
                return 'test'

        module_mock = MagicMock()
        with patch.dict('sys.modules', **{
                'lantern': module_mock,
                'lantern.live': module_mock,
                }):
            module_mock.LanternLive = Test
            from perspective.psp import _type_detect

            x = _type_detect(Test())

            assert x == 'test'

            import sys
            sys.modules['lantern'] = Nope()
            _type_detect('test')

    def test_list(self):
        from perspective.psp import _type_detect
        x = ['a', 'simple', 'test']

        y = _type_detect(x)
        assert y == '["a","simple","test"]'

    def test_dict(self):
        from perspective.psp import _type_detect
        x = {'a': 'simple test'}

        y = _type_detect(x)
        assert y == '[{"a":"simple test"}]'

    def test_pyarrow(self):
        import pyarrow as pa
        from perspective.psp import _type_detect
        x1 = pa.Array(['test', 'test2'])
        x2 = pa.frombuffer(b'test')

        _type_detect(x1)
        _type_detect(x2)

        import sys
        sys.modules['pyarrow'] = Nope()
        _type_detect('test')
        sys.modules['pyarrow'] = pa


    def test_webroutes(self):
        from perspective.psp import _type_detect
        x = ['https://', 'http://', 'wss://', 'ws://', 'sio://']
        for val in x:
            assert val + 'test' == _type_detect(val + 'test')

    def test_other(self):
        from perspective.psp import _type_detect
        x = _type_detect('test')
        assert x == 'test'
