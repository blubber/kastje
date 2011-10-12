
import sys, time, os, serial, sys, socket, select, struct
import os.path, signal

config = {
    'device': '/dev/ttyUSB0',
    'pidfile': '/home/blubber/Workspace/lightcontrol/lights/lightd.pid',
    'logfile': '/home/blubber/Workspace/lightcontrol/lights/lightd.log',
    'uid': 1000,
    'gid': 20,
    'daemon': False,
    }


server_socket = None
sockets = []
running = True
serial_con = None
logfile = None


def send (cmd, payload_len = 0, payload = None):
    global serial_con

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
        serial_con.target.write(chr(cmd))
    else:
        serial_con.write(cmd)

    serial_con.write(chr(payload_len))

    if payload_len > 0:
        for b in payload:
            serial_con.write(b)
        
    c = serial_con.read()
    l = ord(serial_con.read())
    p = ''
    
    while len(p) < l:
        p += serial_con.read(l - len(p))

    return (c, l, p)


def log (msg):
    global logfile

    if config['daemon']:
        if logfile == None:
            logfile = open(config['logfile'], 'a')
        logfile.write('%s:  %s\n' % (time.asctime(), msg))
    else:
        print msg


def parse_config (config_file):
    fp = open(config_file, 'r')
    lines = fp.readlines()
    fp.close()

    for l in map(lambda l: l.strip(), lines):
        if l.startswith('#'): continue

        parts = l.split('=')
        if not len(parts) is 2: continue
        k = parts[0].strip()
        v = parts[1].strip()

        if k in config:
            if type(config[k]) == int:
                try:
                    v = int(v)
                except ValueError, e:
                    print "Unable to parse config key '%s' expected an int" % k
            config[k] = v


def cleanup (arg1 = None, arg2 = None):
    global mainloop
    
    log("Cleaning up")

    running = False

    for s in sockets:
        s.shutdown(socket.SHUT_RDWR)
        s.close()

    if server_socket:
        server_socket.shutdown(socket.SHUT_RDWR)
        server_socket.close()

    if serial_con:
        serial_con.close()
    
    try:
        os.remove('/tmp/lights')
    except OSError, e: pass

    try:
        os.remove(config['pidfile'])
    except OSError, e: pass


def client_socket_listener (sock):
    data = sock.recv(4096)

    if len(data) == 0:
        sock.shutdown(socket.SHUT_RDWR)
        sock.close()
        sockets.remove(sock)
        return False

    cmd = data[0]
    payload_len = ord(data[1])
    if payload_len > 0:
        payload = data[2:]
    else:
        payload = None

    c, l, p = send(cmd, payload_len, payload)

    sock.send(c)
    sock.send(chr(l))
    sock.send(p)


def start (daemon, config_dir):
    global serial_con, sockets
    
    parse_config('%s/daemon.conf' % config_dir)
    if daemon == True: config['daemon'] = True

    if os.path.isfile('/tmp/lights'):
        print "Unix socket exists, is the server running?"
        sys.exit(-1)

    try:
        serial_con = serial.Serial(config['device'], 9600)
    except Exception, e:
        print e
        sys.exit(-1)

    if config['daemon']:
        if not os.getuid() == 0:
            print "You must start the daemon as root."
            sys.exit(-1)

        if os.path.isfile(config['pidfile']):
            print "Pidfile already exists, is the server running?"
            sys.exit(-1)
 
        if not os.path.isfile(config['logfile']):
            open(config['logfile'], 'w').close()

        os.chown(config['logfile'], config['uid'], config['gid'])

        try:
            pid = os.fork()
            if pid > 0:
                sys.exit(0)
        except OSError, e:
            print "Unable to fork."
            sys.exit(-1)

        os.chdir("/")
        os.umask(0)

        if not os.path.isdir(os.path.dirname(config['pidfile'])):
            os.mkdir(os.path.dirname(config['pidfile']))

        os.chown(os.path.dirname(config['pidfile']), config['uid'],
                 config['gid'])

        os.setgid(config['gid'])
        os.setuid(config['uid'])
        os.setsid()
        
        try:
            pid = os.fork()
            if pid > 0:
                fp = open(config['pidfile'], 'w')
                fp.write('%d' % pid)
                fp.close()
                sys.exit(0)
        except OSError, e:
            print "Unable to fork"
            sys.exit(-1)

        #sys.stdout.close()
        #sys.stderr.close()
        #sys.stdin.close()

    server_socket = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_socket.bind('/tmp/lights')
    server_socket.listen(1)
        
    signal.signal(signal.SIGINT, cleanup)
    while running:
        s = sockets + [server_socket]
        try:
            r, _, e = select.select(s, [], s)
        except Exception as e:
            break
        for _s in r:
            if _s == server_socket:
                client_socket, addr = server_socket.accept()
                sockets.append(client_socket)
            else:
                client_socket_listener(_s)
        for _s in e:
            _s.close()
            _s.shutdown()
            if _s == server_socket:
                cleanup()
            else:
                socket.remove(_s)
