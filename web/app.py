#!/usr/bin/env python

from werkzeug.wrappers import Request, Response
from werkzeug.serving import run_simple
from werkzeug.utils import parse_cookie, dump_cookie
from werkzeug.exceptions import HTTPException, NotFound, InternalServerError
from werkzeug.routing import Map, Rule
from werkzeug.wsgi import SharedDataMiddleware

import arduino.client

import os.path
import hashlib
import json

addr = 'localhost'
port = 8080
debug = True
salt = "gflhlcrytycvrneutyrec"
password = hashlib.sha512(salt + "aperture" + salt).hexdigest()

class App (object):

    def api (self, request, args):
        cookies = parse_cookie(request.environ)
        if not 'key' in cookies or not cookies['key'] == password:
            return Response('fail')
        
        func = args['func']
        if func == 'rgb':
            if not 'args' in args:
                return NotFound()
            parts = args['args'].split(':')
            print parts
            try:
                arduino.client.rgb(*parts)
            except Exception as e:
                print e
                return NotFound()

        return Response('')

    def login (self, request, args):
        key = hashlib.sha512(salt + args['key'] + salt).hexdigest()
        if key == password:
            return Response('ok',
                    headers=[('Set-Cookie', dump_cookie('key', key))])
        return Response('failed')

    def index (self, request, args):
        return Response('''<html>
<head>
<title>Lights</title>
<head>
<body>
<script src="/static/lights.js"></script>
</body>
</html>''')

    def __init__ (self):
        arduino.client.init('/etc/lights/')
        self.url_map = Map([
            Rule('/', endpoint=self.index),
            Rule('/api/<func>/', endpoint=self.api),
            Rule('/api/<func>/<args>/', endpoint=self.api),
            Rule('/login/<key>/', endpoint=self.login),
        ])

    def dispatch_request(self, request):
        adapter = self.url_map.bind_to_environ(request.environ)
        try:
            endpoint, args = adapter.match()
            return endpoint(request, args)
        except HTTPException as e:
            print e
            return InternalServerError()
        return NotFound()

    def __call__ (self, environ, start_response):
        request = Request(environ)
        response = self.dispatch_request(request)
        return response(environ, start_response)

if __name__ == '__main__':
    app = SharedDataMiddleware(App(), {
        '/static': os.path.join(os.path.dirname(__file__), 'static')
    })
    run_simple(addr, port, app, use_reloader=debug)
