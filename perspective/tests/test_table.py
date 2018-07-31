

class TestTable:
    def setup(self):
        pass
        # setup() before each test method

    def test_table(self):
        import numpy as np
        from perspective.table import Perspective

        t = Perspective(['Col1', 'Col2', 'Col3', 'Col4', 'Col5'], [int, str, float, np.int64, np.float64])
        t.load('Col1', [1, 2, 3, 4])
        t.load('Col2', ["abcd", "defg", "csdf", "dasf"])
        t.load('Col3', [1.1, 2.2, 3.3, 4.4])

        arr1 = np.array([1, 2, 3, 4])
        arr2 = np.array([1.1, 2.2, 3.3, 4.4])

        t.load('Col4', arr1)
        t.load('Col5', arr2)

        t.print()
