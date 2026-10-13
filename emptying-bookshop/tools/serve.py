#!/usr/bin/env python3
"""Serve the film for local viewing with browser caching turned off.

usage: python3 tools/serve.py [port]      (default port 8017)

Plain `python3 -m http.server` lets the browser cache files heuristically, so a browser
that has visited another project on the same port can keep using that project's files.
Sending `Cache-Control: no-store` (and using a port of its own) avoids that.
"""
import functools
import http.server
import os
import socket
import sys

PORT = int(sys.argv[1]) if len(sys.argv) > 1 else 8017
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


class NoCacheHandler(http.server.SimpleHTTPRequestHandler):
    def end_headers(self):
        self.send_header('Cache-Control', 'no-store')
        super().end_headers()


class DualStackServer(http.server.ThreadingHTTPServer):
    # Listen on IPv6 and IPv4 so both localhost (::1) and 127.0.0.1 work.
    address_family = socket.AF_INET6

    def server_bind(self):
        try:
            self.socket.setsockopt(socket.IPPROTO_IPV6, socket.IPV6_V6ONLY, 0)
        except (AttributeError, OSError):
            pass
        super().server_bind()


def main():
    handler = functools.partial(NoCacheHandler, directory=ROOT)
    try:
        server = DualStackServer(('::', PORT), handler)
    except OSError:
        server = http.server.ThreadingHTTPServer(('', PORT), handler)
    with server:
        print(f'The Emptying Bookshop: http://localhost:{PORT}/  (Ctrl+C to stop)', flush=True)
        try:
            server.serve_forever()
        except KeyboardInterrupt:
            pass


if __name__ == '__main__':
    main()
