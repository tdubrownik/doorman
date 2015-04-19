from time import sleep
from sys import stderr

import serial

from command import Command
import options

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
        print cmd
        for i in cmd:
            sleep(0.02)
            self.fd.write(i)
    def recv(self):
        line = self.fd.readline()
        print line
        if line[0] != '$':
            return self.recv()
        cmd = Command.from_str(line)
        if cmd.command == 'E':
            raise RemoteException(cmd.hash, cmd.uid)
        return cmd

class MockProto(object):
    def __init__(*a, **kw):
        pass
    def send(self, command):
        pass
    def recv(self):
        return Command()
