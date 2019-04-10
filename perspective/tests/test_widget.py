
class TestWidget:
    def test_json(self):
        from perspective.widget import PerspectiveWidget
        pw = PerspectiveWidget([])
        print(pw._as_json())
        assert pw._as_json()

    def test_computedColumns1(self):
        from perspective.widget import PerspectiveWidget

        PerspectiveWidget([{'test': 1, 'test2': 2}], computedcolumns={'inputs': ['test', 'test2'], 'func': 'add'})
        PerspectiveWidget([{'test': 1, 1: 2}], computedcolumns={'inputs': ['test', 1], 'func': 'add'})

    def test_computedColumns2(self):
        from perspective.widget import PerspectiveWidget
        from perspective import PSPException

        try:
            PerspectiveWidget([{'test': 1, 'test2': 2}], computedcolumns={'inputs': ['test', 'test2'], 'func': 'sdfgsdfgsdf'})
            assert False
        except PSPException:
            pass

    # def test_computedColumns3(self):
    #     from perspective.widget import PerspectiveWidget
    #     from perspective import PSPException

    #     try:
    #         PerspectiveWidget([{'test': 1, 'test2': 2}], computedcolumns={'inputs': ['test', 'test2dfgsdfg'], 'func': 'add'})
    #         assert False
    #     except PSPException:
    #         pass

    def test_computedColumns4(self):
        from perspective.widget import PerspectiveWidget
        from perspective import PSPException

        try:
            PerspectiveWidget([{'test': 1, 'test2': 2}], computedcolumns={'inputs': ['test', 'test2']})
            assert False
        except PSPException:
            pass

    def test_computedColumns5(self):
        from perspective.widget import PerspectiveWidget
        from perspective import PSPException

        try:
            PerspectiveWidget([{'test': 1, 'test2': 2}], computedcolumns=5)
            assert False
        except PSPException:
            pass

    def test_computedColumns6(self):
        from perspective.widget import PerspectiveWidget
        from perspective import PSPException

        try:
            PerspectiveWidget([{'test': 1, 'test2': 2}], computedcolumns={'inputs': {}, 'func': 'add'})
            assert False
        except PSPException:
            pass
