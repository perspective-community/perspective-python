
class HTTPHelper(object):
    def __init__(self, url, field='', records=False, repeat=-1):
        self.url = url
        self.field = field
        self.records = records
        self.repeat = repeat


class WSHelper(object):
    def __init__(self, url, send=None, records=False):
        self.url = url
        self.send = send
        self.records = records


class SIOHelper(object):
    def __init__(self, url, channel='', records=False):
        self.url = url
        self.channel = channel
        self.records = records
