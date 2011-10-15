
import UserDict
import os, os.path
import json


default_config = {
    'device': '/dev/ttyUSB0',
    'pidfile': '/var/run/lights/lightd.pid',
    'logfile': '/var/log/lights/lightd.log',
    'uid': 1000,
    'gid': 20,
    'daemon': False,
    'address': '-1',
    'port': 12345,
}


class Config (UserDict.UserDict):

    def __init__ (self):
        """Inits the configuration system.

        In case of a parsing error a ValueError will be thrown,
        the caller is respnsible for handling this condition. If 
        an unknown key is encountered or a value has the wrong
        type a ValueError is thrown.

        """
        UserDict.UserDict.__init__ (self)
        self.confdir = os.getenv("LIGHTS_CONFDIR", "/etc/lights/")
        configfile = os.path.join(self.confdir, 'lights.conf')

        if not os.path.isfile(configfile):
            configfile = './lights.conf'
            if not os.path.isfile(configfile):
                return

        self.data = default_config
        with open(configfile, 'r') as fp:
            data = json.loads(fp.read())

        for k, v in data.items():
            if not k in default_config:
                raise ValueError("Unknown key '%s'" % k)
            if type(v) is unicode:
                v = v.encode("ascii")
            if not type(v) is type(default_config[k]):
                raise ValueError("Wrong parameter type for key '%s'" % k)
            self.data[k] = v 
