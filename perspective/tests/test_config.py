
class TestConfig:
    def setup(self):
        pass
        # setup() before each test method

    def test_config(self):
        from perspective._config import config
        assert config('', None) == '{}'
        assert config('{}', 'test') == '{}'
        print(config({}, 'http://'))
        assert config({}, 'http://') == '{"field": "", "records": true, "repeat": -1}'
        assert config({}, 'https://') == '{"field": "", "records": true, "repeat": -1}'
        assert config({}, 'ws://') == '{"send": "{}", "records": true}'
        assert config({}, 'wss://') == '{"send": "{}", "records": true}'
        assert config({}, 'sio://') == '{"channel": "", "records": true}'
