import serial
import options
from command import Command
from time import sleep
from sys import stderr

class RemoteException(Exception):
    pass
class Proto(object):
    def __init__(self, url=None, **kwargs):
        kwa = {}
        kwa.update(options.serial)
        kwa.update(kwargs)
        url = url or options.url
        self.fd = serial.serial_for_url(url, **kwa)
        sleep(options.init_sleep)
        self.fd.flushInput()
        self.fd.flushOutput()
        print >> stderr, 'Serial port ready'
    def send(self, command):
        cmd = str(command) + '\n'
        self.fd.write(cmd)
    def recv(self):
        line = self.fd.readline()
        cmd = Command.from_str(line)
        if cmd.command == 'E':
            raise RemoteException(cmd.mid, cmd.uid)
        return cmd
