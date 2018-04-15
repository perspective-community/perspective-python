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
            df = pd.DataFrame([1, 2], columns=['1'])
            psp(df)
            assert m1.call_count == 1

    def test_layout(self):
        import pandas as pd
        from perspective import psp, View, PSPException
        with patch('IPython.display.display'):
            df = pd.DataFrame([1, 2], columns=['1'])
            psp(df, View.YBAR)
            psp(df, 'y_line')
            try:
                psp(df, 'test')
                assert False
            except PSPException:
                pass

            try:
                psp(df, 5)
                assert False
            except PSPException:
                pass

    def test_layout2(self):
        import pandas as pd
        from perspective import psp, View, PSPException
        with patch('IPython.display.display'):
            df = pd.DataFrame([1, 2], columns=['1'])
            psp(df, View.YBAR, '1')
            psp(df, View.YBAR, ['1'])
            try:
                psp(df, View.YBAR, 5)
                assert False
            except PSPException:
                pass

    def test_layout3(self):
        import pandas as pd
        from perspective import psp, View, PSPException
        with patch('IPython.display.display'):
            df = pd.DataFrame([1, 2], columns=['1'])
            psp(df, View.YBAR, '1')
            psp(df, View.YBAR, ['1'])
            psp(df, View.YBAR, ['1'], '1')
            psp(df, View.YBAR, ['1'], ['1'])
            try:
                psp(df, View.YBAR, ['1'], 5)
                assert False
            except PSPException:
                pass

    def test_layout4(self):
        import pandas as pd
        from perspective import psp, View, PSPException
        with patch('IPython.display.display'):
            df = pd.DataFrame([1, 2], columns=['1'])
            psp(df, View.YBAR, ['1'])
            psp(df, View.YBAR, ['1'], None, ['1'])
            psp(df, View.YBAR, ['1'], None, '1')
            try:
                psp(df, View.YBAR, ['1'], None, 5)
                assert False
            except PSPException:
                pass

    def test_layout5(self):
        import pandas as pd
        from perspective import psp, View, PSPException
        with patch('IPython.display.display'):
            df = pd.DataFrame([1, 2], columns=['1'])
            psp(df, View.YBAR, ['1'])
            psp(df, View.YBAR, ['1'], None, None, None, 'test')
            psp(df, View.YBAR, ['1'], None, None, None, ['test'])
            try:
                psp(df, View.YBAR, ['1'], None, None, None, 5)
                assert False
            except PSPException:
                pass

    def test_aggregates(self):
        import pandas as pd
        from perspective import psp, View, Aggregate, PSPException
        with patch('IPython.display.display'):
            df = pd.DataFrame([1, 2], columns=['1'])
            psp(df, View.YBAR, ['1'], None, ['1'], {'1': Aggregate.ANY})
            psp(df, View.YBAR, ['1'], None, ['1'], {'1': 'any'})
            try:
                psp(df, View.YBAR, ['1'], None, ['1'], {'1': 'test'})
                assert False
            except PSPException:
                pass
            try:
                psp(df, View.YBAR, ['1'], None, ['1'], {'1': 5})
                assert False
            except PSPException:
                pass
            try:
                psp(df, View.YBAR, ['1'], None, ['1'], 5)
                assert False
            except PSPException:
                pass

    def test_config(self):
        from perspective.psp import _config
        assert _config('', None) == ''
        assert _config('{}', 'test') == ''
        assert _config({}, 'http://') == '{"field":"","records":false,"repeat":10}'
        assert _config({}, 'https://') == '{"field":"","records":false,"repeat":10}'
        assert _config({}, 'ws://') == '{"send":"{}","records":false}'
        assert _config({}, 'wss://') == '{"send":"{}","records":false}'
        assert _config({}, 'sio://') == '{"channel":"","records":false}'
