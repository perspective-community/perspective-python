from .base import PerspectiveBaseMixin


class PerspectiveHTTPMixin(object):
    def loadData(self, transfer_as_arrow=False, **kwargs):
        # Override
        kwargs['transfer_as_arrow'] = transfer_as_arrow

        self.psp = PerspectiveBaseMixin()
        self.psp.setup(**kwargs)

    def getData(self, data_only=False):
        return self.psp._as_json(data_only=data_only, allow_nan=False)


class PerspectiveWSMixin(object):
    def loadData(self, **kwargs):
        self.psp = PerspectiveBaseMixin()
        self.psp.setup(**kwargs)

    def stream_open(self):
        print("opened")

    def stream_message(self, message):
        self.write_message(u"You said: " + message)

    def streams_close(self):
        print("Closed")

    def getData(self, data_only=False):
        return self.psp._as_json(data_only=data_only, allow_nan=False)
