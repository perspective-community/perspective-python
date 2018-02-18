from mock import patch, MagicMock
from datetime import datetime


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

    def test_other(self):
        from perspective.psp import _type_detect
        x = _type_detect('test')
        assert x == 'test'
