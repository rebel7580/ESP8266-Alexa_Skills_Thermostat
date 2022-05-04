import websockets
import asyncio
import json
import time, os

class HttpWSSProtocol(websockets.WebSocketServerProtocol):
    rwebsocket = None
    rddata = None
    async def handler(self):
        try:
            request_line, headers = await websockets.http.read_message(self.reader)
            method, path, version = request_line[:-2].decode().split(None, 2)
            #websockets.accept()
        except Exception as e:
            print(e.args)
            self.writer.close()
            self.ws_server.unregister(self)

            raise

        # TODO: Check headers etc. to see if we are to upgrade to WS.
        if path == '/ws':
            # HACK: Put the read data back, to continue with normal WS handling.
            self.reader.feed_data(bytes(request_line))
            self.reader.feed_data(headers.as_bytes().replace(b'\n', b'\r\n'))

            return await super(HttpWSSProtocol, self).handler()
        else:
            try:
                return await self.http_handler(method, path, version)
            except Exception as e:
                print(e)
            finally:

                self.writer.close()
                self.ws_server.unregister(self)


    async def http_handler(self, method, path, version):
        response = ''
        try:

            Request = self.reader._buffer.decode('utf-8')
            RequestJson = json.loads(Request)
            ESPparameters = {}
            command = RequestJson['request']['intent']['slots']
         
            if 'value' in command['question'].keys():
                ESPparameters['query'] = '?'
            else:
                ESPparameters['query'] = 'cmd'

            if 'value' in command['state'].keys():
                if 'resolutions' in command['state'].keys():
                    ESPparameters['state'] = command['state']['resolutions']['resolutionsPerAuthority'][0]['values'][0]['value']['name']
                else:
                    ESPparameters['state'] = command['state']['value']
            else:
                ESPparameters['state'] = 'none'

            if 'value' in command['setting'].keys():
                if 'resolutions' in command['setting'].keys():
                    ESPparameters['setting'] = command['setting']['resolutions']['resolutionsPerAuthority'][0]['values'][0]['value']['name']
                else:
                    ESPparameters['setting'] = command['setting']['value']
            else:
                ESPparameters['setting'] = 'none'

            if 'value' in command['instance'].keys():
                if 'resolutions' in command['instance'].keys():
                    ESPparameters['instance'] = command['instance']['resolutions']['resolutionsPerAuthority'][0]['values'][0]['value']['name']
                else:
                    ESPparameters['instance'] = command['instance']['value']
            else:
                ESPparameters['instance'] = 'none'



            # {"instance": "temperature", "setting": "none", "state": "none", "query":"?"}
            # {"instance": "none", "setting": "none", "state": "heat", "query":"cmd"}
            # {"instance": "temperature", "setting": "58", "state": "none", "query":"cmd"}

            # # send command to ESP over websocket
            if self.rwebsocket== None:
                print("Device is not connected!")
                return
            #await self.rwebsocket.send(json.dumps(googleRequestJson))
            await self.rwebsocket.send(json.dumps(ESPparameters))
            #wait for response and send it back to Alexa as is
            self.rddata = await self.rwebsocket.recv()

            response = '\r\n'.join([
                'HTTP/1.1 200 OK',
                'Content-Type: text/json',
                '',
                ''+self.rddata+'',
            ])
        except Exception as e:
            print(e)
        self.writer.write(response.encode())

def updateData(data):
    HttpWSSProtocol.rddata = data

async def ws_handler(websocket, path):
    game_name = 'g1'
    try:
        HttpWSSProtocol.rwebsocket = websocket
        await websocket.send(json.dumps({'event': 'OK'}))
        data ='{"empty":"empty"}'
        while True:
            data = await websocket.recv()
            updateData(data)
    except Exception as e:
        print(e)
    finally:
        print("")

def _read_ready(self):
    if self._conn_lost:
        return
    try:
        time.sleep(.1)
        data = self._sock.recv(self.max_size)
    except (BlockingIOError, InterruptedError):
        pass
    except Exception as exc:
        self._fatal_error(exc, 'Fatal read error on socket transport')
    else:
        if data:
            self._protocol.data_received(data)
        else:
            if self._loop.get_debug():
                print("%r received EOF")
            keep_open = self._protocol.eof_received()
            if keep_open:
                # We're keeping the connection open so the
                # protocol can write more, but we still can't
                # receive more, so remove the reader callback.
                self._loop._remove_reader(self._sock_fd)
            else:
                self.close()

asyncio.selector_events._SelectorSocketTransport._read_ready = _read_ready

port = int(os.getenv('PORT', 80))#5687
start_server = websockets.serve(ws_handler, '', port, klass=HttpWSSProtocol)
# logger.info('Listening on port %d', port)

asyncio.get_event_loop().run_until_complete(start_server)
asyncio.get_event_loop().run_forever()
