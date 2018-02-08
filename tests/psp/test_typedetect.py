
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

    def test_lantern(self):
        from lantern.live import LanternLive
        from perspective.psp import _type_detect

        class Test(LanternLive):
            def __init__(self):
                pass

            def path(self):
                return 'test'

        x = _type_detect(Test())

        assert x == 'test'
