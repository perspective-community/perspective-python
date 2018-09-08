import asyncio
import tornado.httpclient


class GenBase(object):
    def validate(self, url, type):
        if type == 'http':
            if not url.startswith('http://') or url.startswith('https://'):
                raise Exception('Invalid url for type http: %s' % url)
        elif type == 'ws':
            if not url.startswith('ws://') or url.startswith('wss://'):
                raise Exception('Invalid url for type ws: %s' % url)
        elif type == 'sio':
            if not url.startswith('sio://'):
                raise Exception('Invalid url for type socketio: %s' % url)

    @asyncio.coroutine
    def run(self):
        for item in self.getData():
            self.psp.update(item)

    def start(self):
        loop = asyncio.get_event_loop()

        try:
            loop.run_until_complete(self.run())

        finally:
            loop.close()


class HTTPHelper(GenBase):
    def __init__(self, psp, url, field='', records=False, repeat=-1):
        self.validate(url, 'http')
        self.__type = 'http'
        self.url = url
        self.field = field
        self.records = records
        self.repeat = repeat
        self.psp = psp

    @asyncio.coroutine
    def getData(self):
        client = tornado.httpclient.AsyncHTTPClient()
        dat = client.fetch(self.url)
        if self.field:
            dat = dat[self.field]
        if self.records is False:
            dat = [dat]
        yield asyncio.From(dat)
        while(self.repeat >= 0):
            asyncio.sleep(self.repeat)
            dat = client.fetch(self.url)
            if self.field:
                dat = dat[self.field]
            if self.records is False:
                dat = [dat]
            yield asyncio.From(dat)


class WSHelper(GenBase):
    def __init__(self, psp, url, send=None, records=False):
        self.validate(url, 'ws')
        self.__type = 'ws'
        self.url = url
        self.send = send
        self.records = records

    @asyncio.coroutine
    def getData(self):
        pass


class SIOHelper(GenBase):
    def __init__(self, psp, url, channel='', records=False):
        self.validate(url, 'sio')
        self.__type = 'sio'
        self.url = url
        self.channel = channel
        self.records = records

    @asyncio.coroutine
    def getData(self):
        pass
