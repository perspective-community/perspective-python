from mock import patch


class TestPSP:
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

    def test_psp(self):
        import pandas as pd
        from perspective import psp
        with patch('IPython.display.display') as m1:
            df = pd.DataFrame([1, 2])
            psp(df)
            assert m1.call_count == 1
