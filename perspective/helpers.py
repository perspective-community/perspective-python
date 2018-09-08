import asyncio
import json
import websockets
import tornado.httpclient


class GenBase(object):
    def validate(self, url, type):
        if type == 'http':
            if not url.startswith('http://') and not url.startswith('https://'):
                raise Exception('Invalid url for type http: %s' % url)
        elif type == 'ws':
            if not url.startswith('ws://') and not url.startswith('wss://'):
                raise Exception('Invalid url for type ws: %s' % url)
        elif type == 'sio':
            if not url.startswith('sio://'):
                raise Exception('Invalid url for type socketio: %s' % url)

    # @asyncio.coroutine
    # def run(self):
    #     while True:
    #         item = yield from self.getData()
    #         self.psp.update(item)

    async def run(self):
        async for item in self.getData():
            self.psp.update(item)

    def start(self):
        loop = asyncio.get_event_loop()

        try:
            if not loop.is_running():
                loop.run_until_complete(self.run())
            else:
                asyncio.ensure_future(self.run(), loop=loop)
        except KeyboardInterrupt:
            loop.close()


class HTTPHelper(GenBase):
    def __init__(self, psp, url, field='', records=True, repeat=-1):
        self.validate(url, 'http')
        self.__type = 'http'
        self.url = url
        self.field = field
        self.records = records
        self.repeat = repeat
        self.psp = psp

    # @asyncio.coroutine
    async def getData(self):
        client = tornado.httpclient.AsyncHTTPClient()
        dat = await client.fetch(self.url)
        dat = json.loads(dat.body)

        if self.field:
            dat = dat[self.field]
        if self.records is False:
            dat = [dat]
        # yield asyncio.From(dat)
        yield dat

        while(self.repeat >= 0):
            asyncio.sleep(self.repeat)
            dat = await client.fetch(self.url)
            if self.field:
                dat = dat[self.field]
            if self.records is False:
                dat = [dat]
            yield dat


class WSHelper(GenBase):
    def __init__(self, psp, url, send=None, records=True):
        self.validate(url, 'ws')
        self.__type = 'ws'
        self.url = url
        self.send = send
        self.records = records
        self.psp = psp

    # @asyncio.coroutine
    async def getData(self):
        # websocket = yield from websockets.connect(self.url)
        async with websockets.connect(self.url) as websocket:
            if self.send:
                # yield from websocket.send(self.send)
                await websocket.send(self.send)

            # data = yield from websocket.recv()
            data = await websocket.recv()

            if self.records is False:
                yield [data]
            else:
                yield data


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
