from .base import PerspectiveBaseMixin


class PerspectiveHTTPMixin(object):
    def loadData(self, data=None, options=None):
        options = options or {}
        options['data'] = data
        self.psp = PerspectiveBaseMixin()
        self.psp.setup(**options)

    def getData(self, data_only=False):
        return self.psp._as_json(data_only=data_only)
