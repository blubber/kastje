
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
        self.kaku_aliases = {}
        self.led_aliases = {}
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
            if k == "kaku":
                for k, v in data['kaku'].items():
                    try:
                        self.kaku_aliases[k.encode('ascii')] = {
                            'group': int(v['group']),
                            'module': int(v['module']),
                        }
                    except ValueError as e:
                        raise ValueError("Group and module must be integers!")
                    except KeyError as e:
                        raise KeyError("Group and module arguments are required!")
                continue
            elif k == "leds":
                for k, v in data["leds"].items():
                    try:
                        self.led_aliases[k.encode('ascii')] = int(v)
                    except ValueError as e:
                        raise ValueError("Led alias argument must be an integer!")
                continue
            elif not k in default_config:
                raise ValueError("Unknown key '%s'" % k)
            if type(v) is unicode:
                v = v.encode("ascii")
            if not type(v) is type(default_config[k]):
                raise ValueError("Wrong parameter type for key '%s'" % k)
            self.data[k] = v 

    def led_alias (self, k):
        if k in self.led_aliases:
            return self.led_aliases[k]
        else:
            return None

    def kaku_alias (self, k):
        if k in self.kaku_aliases:
            return (self.kaku_aliases[k]['group'],
                    self.kaku_aliases[k]['module'])
        else:
            return None
