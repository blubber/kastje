import socket, struct, re, sys
import arduino.utils

class ArduinoError(Exception): pass
class InvalidArgument(ArduinoError): pass
class UnknownAlias(ArduinoError): pass

config = arduino.utils.Config()
presets = arduino.utils.Presets()

class Message (object):
    """ A general purpose Message class """

    # GROUP 0 -- System
    MSG_DEBUG       = 0x00
    MSG_ACK         = 0x01
    MSG_IDENT       = 0x02
    MSG_UNKNOWN     = 0x03

    # GROUP 1 -- LEDs
    MSG_RGB         = 0x10
    MSG_CYCLE       = 0x11

    # GROUP 2 -- Kaku
    MSG_ON          = 0x20
    MSG_OFF         = 0x21
    
    def __init__ (self, cmd, payload_len = 0, payload = None):
        self.cmd = type(cmd) == int and cmd or ord(cmd)
        self.payload_len = payload_len
        self.payload = payload

    def send (self):
        try:
            if config['address'] == '-1':
                sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
                sock.connect('/tmp/lights')
            else:
                sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                sock.connect((config['address'], config['port']))
        except Exception, e:
            raise ArduinoError("Unable to connect")

        sock.send(struct.pack('BB', self.cmd, self.payload_len))
        if self.payload_len > 0:
            sock.send(self.payload)
        
        data = ''
        while True:
            _data = sock.recv(64)
            if len(_data) == 0:
                break
            data += _data
            if len(data) >= 2:
                if len(data) >= 2 + ord(data[1]):
                    break

        sock.shutdown(socket.SHUT_RDWR)
        sock.close()

    

def kaku (state, *args):
    """ Turn a kaku switch on or off

    Arguments:
       state: True to turn on False to turn off
       args:  If args is a single string it is treated as a
              kaku alias. (See client config file for details).

              In case args is a single int it is treated as a group,
              and the whole group will be set to state.

              Args can also specify a (group, module) tuple.

    Raises:
        AliasUnknown: in case an unknown alias is specified.
        InvalidArgument: in case the group and/or module is out of range.
        ArduiniError: in all other cases
    """
    group = None
    module = None

    if len(args) == 1:
        try:
            group = int(args[0])
        except ValueError, e:
            alias = config.kaku_alias(args[0].lower().strip())
            if alias:
                group = alias[0]
                module = alias[1]
            else:
                raise UnknownAlias("Alias '%s' unknown" % alias)
    elif len(args) == 2:
        try:
            group = int(args[0])
            module = int(args[1])
        except ValueError, e:
            InvalidArgument('Group and module must be integers.')
    else:
        raise InvalidArguments('Too many arguments.')
        
    if group is None:
        raise InvalidArgument("Invalid module specified.")

    if group < 1 or module > 4:
        raise InvalidArgument("Group must be between 1 and 4 (inclusive)")

    if module is None:
        for i in range(1, 5):
            Message(state and Message.MSG_ON or Message.MSG_OFF,
                    2, struct.pack('BB', group, i)).send()
    else:
        if module < 1 or module > 4:
            raise InvalidArgument("Module must be between 1 and 4 (inclusive)")
        Message(state and Message.MSG_ON or Message.MSG_OFF,
                2, struct.pack('BB', group, module)).send()

          
def rgb (*args):
    """ Set RGB colors on 1 or more ledgroups

    Arguments:
        This function takes either 3 or 4 arguments. In
        both cases the last 3 arguments are R, G and B values
        ranging from 0-255 (inclusive).

        If an additional (first) argument is supplied it is
        treated as ledgroup id or alias.

        If only three arguments are supplied, the values are
        applied to all ledgroup aliasses.

    Raises:
        InvalidArgument: if one of the supplied arguments is out of range
                         or otherwise invalid.
        UnknownAlias: if the specified alias is unknown.
        ArduiniError: on any other errors.
    """
    group = None
    rgb = ()
    offset = 0
    
    if len(args) == 4:
        offset = 1
        try:
            group = int(args[0])
        except ValueError, e:
            alias = config.led_alias(args[0].lower().strip())
            if not alias is None:
                group = alias
            else:
                raise UnknownAlias("Ledgroup '%s' unknwon." % alias)
    elif len(args) > 4:
        raise InvalidArgument("Too many arguments.")
    elif len(args) < 3:
        raise InvalidArgument("Not enough arguments.")
    
    try:
        rgb = (int(args[offset]), int(args[offset+1]), int(args[offset+2]))
    except ValueError, e:
        raise InvalidArgument("R, G and B must be integers")

    if len(filter(lambda x: not (x >= 0 and x <= 255), rgb)) > 0:
        raise ValueError("R, G and B must be between 0 and 255 (inclusive).")

    if group == None:
        for g in config['ledgroups']:
            Message(Message.MSG_RGB, 4,
                    struct.pack('BBBB',
                                config['ledgroups'][g],
                                rgb[0], rgb[1], rgb[2])).send()
    else:
        Message(Message.MSG_RGB, 4, struct.pack('BBBB', group,
                                                rgb[0], rgb[1], rgb[2])).send()

def cycle (*args):
    """ Cycle colors.

    Arguments:
        This function takes either 1, 2, 3 or 4:

            1 argument: the argument is treated as the speed (see below).
            2 arguments: group/alias and speed.
            3 arguments: group/alias, lightness and speed
            4 arguments: group/alias, lightness, saturation and speed
    Raises:
        UnknownAlias: in case the specified alias is unknown.
        InvalidArgument: if the specified values are out of range or
                         otherwise invalid.
        ArduinoError: on any other errrors.

    About the arguments:
        - Speed must be an integer from 1 to 255 (inclusive), it specifies the
          number of milliseconds between each 0.001 increment of the Hue value.

        - Lightness is a value between 0 and 255 (inclusive), it specifies
          the lightness (brightness) of the cycle colors, 0 is black 255
          is white.
          
        - Saturation is a value between 0 and 255 (inclusive), it specifies
          the color saturation.
    """
    group = None
    speed = None
    lightness = 128
    saturation = 255

    if len(args) == 1:
        speed = args[0]
    elif len(args) == 2:
        group = args[0]
        speed = args[1]
    elif len(args) == 3:
        group = args[0]
        lightness = args[1]
        speed = args[2]
    elif len(args) == 4:
        group = args[0]
        lightness = args[1]
        saturation = args[2]
        speed = args[3]

    if group:
        try:
            group = int(group)
        except ValueError, e:
            alias = config.led_alias(group.lower().strip())
            if not alias is None:
                group = alias
            else:
                raise UnknownAlias("Ledgroup '%s' unknown." % alias)

    try:
        speed = int(speed)
        lightness = int(lightness)
        saturation = int(saturation)
    except ValueError, e:
        raise InvalidArguments("Need integer arguments.")

    if group and (group < 0 or group > 255):
        raise InvalidArgument("Group must be between 0 and 255 (inclusive)")
    if saturation < 0 or saturation > 255:
        raise InvalidArgument("Saturation must be between 0 " \
                                  + "and 255 (inclusive)")
    if lightness < 0 or lightness > 255:
        raise InvalidArgument("Lightness must be between 0 " \
                                  + "and 255 (inclusive)")
    if speed < 1 or saturation > 255:
        raise InvalidArgument("Speed must be between 0 " \
                                  + "and 255 (inclusive)")

    if group == None:
        for g in config['ledgroups']:
            Message(Message.MSG_CYCLE, 4,
                    struct.pack('BBBB',
                                config['ledgroups'][g],
                                speed, saturation, lightness)).send()
    else:
        Message(Message.MSG_CYCLE, 4,
                struct.pack('BBBB', group, 
                            speed, saturation, lightness)).send()


def set_preset (preset):
    preset = preset.lower()
    if preset not in presets:
        raise ArduinoError('Unknown preset')
        
    for item in presets[preset]:
        if item[0] == 'led':
            if item[1] == 'rgb':
                rgb(*item[2:])
            elif item[1] == 'cycle':
                cycle(*item[2:])
        elif item[0] == 'kaku':
            kaku(int(item[1]), *[int(x) for x in item[2:]])


def commit_presets (config_dir):
    """ Commits presets to the preset file. """
    pass
