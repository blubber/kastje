
import sys, time, os, serial, sys, socket, select, struct
import os.path, signal
import threading
from arduino.utils import Config


unix_socket = None
tcp_socket = None
running = True
serial_con = None
logfile = None
serial_lock = threading.Lock()
clients = {}
config = Config()


class Client (object):
    
    def __init__ (self, socket):
        self.socket = socket
        self.buffer = ''

    def close (self):
        del clients[self.socket]
        try:
            self.socket.shutdown(socket.RDWR)
        except Exception as e:
            pass
        self.socket.close()

    def read (self):
        try:
            data = self.socket.recv(32)
        except Exception as e:
            print e
            self.close()
            return

        if len(data) == 0:
            self.close()
            return

        self.buffer += data
        self._parse_buffer()

    def _parse_buffer (self):
        if len(self.buffer) < 2:
            return
        cmd, _len = struct.unpack('BB', self.buffer[0:2])
        if len(self.buffer) < 2 + _len:
            return
        payload = self.buffer[2:2+_len]
        self.buffer = self.buffer[2+_len:]
        c, l, p = send(cmd, _len, payload)
        try:
            self.socket.send(c)
            self.socket.send(chr(l))
            self.socket.send(p)
        except Exception as e:
            print e
            self.close()


def send (cmd, payload_len = 0, payload = None):
    global serial_con
    
    if not serial_lock.acquire(False):
        return

    if not serial_con:
        try:
            serial_con = serial.Serial(config['device'], 9600)
        except Exception, e:
            log("Unable to open serial port '%s': %s" % \
                    (config['device'], e))
            sys.exit(-1)

    if not serial_con.isOpen():
        try:
            serial_con.open()
        except Exception, e:
            log("Unable to reopen serial connection '%s': %s" % \
                    (config['device'], e))
            sys.exit(-1)
    
    if type(cmd) == int:
        serial_con.write(chr(cmd))
    else:
        serial_con.write(cmd)

    serial_con.write(chr(payload_len))

    if payload_len > 0:
        for b in payload:
            serial_con.write(b)
    
    serial_con.flush()        
    c = serial_con.read()
    l = ord(serial_con.read())
    p = ''
    
    while len(p) < l:
        p += serial_con.read(l - len(p))
    
    time.sleep(0.01) # Give the ardiuno some time to process. 
    serial_lock.release()
    return (c, l, p)


def log (msg):
    global logfile

    if config['daemon']:
        if logfile == None:
            logfile = open(config['logfile'], 'a')
        logfile.write('%s:  %s\n' % (time.asctime(), msg))
    else:
        print msg


def cleanup (arg1 = None, arg2 = None):
    global clients
    log("Cleaning up")

    running = False

    for socket in clients:
        try:
            socket.shutdown(socket.SHUT_RDWR)
        except Exception as e:
            pass
        socket.close()

    if unix_socket:
        unix_socket.shutdown(socket.SHUT_RDWR)
        unix_socket.close()
    if tcp_socket:
        tcp_socket.shutdown(socket.SHUT_RDWR)
        tcp_socket.close()

    if serial_con:
        serial_con.close()
    
    try:
        os.remove('/tmp/lights')
    except OSError, e: pass

    try:
        os.remove(config['pidfile'])
    except OSError, e: pass


def start ():
    global serial_con, sockets
    
    if os.path.isfile('/tmp/lights'):
        print "Unix socket exists, is the server running?"
        sys.exit(-1)

    try:
        serial_con = serial.Serial(config['device'], 9600)
    except Exception, e:
        print e
        sys.exit(-1)

    unix_socket = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    unix_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    unix_socket.bind('/tmp/lights')
    unix_socket.listen(1)
    server_sockets = [unix_socket]
    
    if not config['address'] == '-1':
        tcp_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        tcp_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        tcp_socket.bind((config['address'], config['port']))
        tcp_socket.listen(1)
        server_sockets.append(tcp_socket)
        
    signal.signal(signal.SIGINT, cleanup)
    while running:
        sockets = clients.keys() + server_sockets
        r, _, e = select.select(sockets, [], sockets, 1)
        for s in e:
            if s in server_sockets:
                cleanup()
            else:
                clients[s].close()
        for s in r:
            if s in server_sockets:
                client_socket, addr = s.accept()
                clients[client_socket] = Client(client_socket)
            else:
                clients[s].read()

    if running:
        cleanup()
