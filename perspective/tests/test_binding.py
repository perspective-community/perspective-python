
class TestBinding:
    def setup(self):
        pass
        # setup() before each test method

    def test_config(self):
        from perspective.table import Perspective
        t = Perspective(['Col1', 'Col2', 'Col3', 'Col4'], [int, str, float, bool])
        t.load('Col1', [1, 2, 3, 4])
        t.load('Col2', ['1', '2', '3', '4'])
        t.load('Col3', [1.1, 2.2, 3.3, 4.4])
        # t.load('Col4', [True, True, False, False])
