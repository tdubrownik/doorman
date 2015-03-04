from collections import namedtuple
import hmac
import hashlib

from options import *

Property = namedtuple('Property', ['name', 'to_str', 'from_str', 'length', 'default'])

def field(name, to_str, from_str, length, default=None):
    return Property(name, to_str, from_str, length, default)

Hex = lambda name, length: field(
    name,
    lambda v: '%0.*x' % (length, v),
    lambda s: int(s, 16),
    length,
    0,
)
Char = lambda name, length: field(
    name,
    lambda v: str(v)[:length],
    lambda s: s,
    length,
    '0' * length,
)
class MalformedException(Exception):
    pass
def check_const(expect, v):
    if expect != v:
        raise MalformedException('expected %s, got %s' % (expect, v))
    return expect
Const = lambda v: field(
    None,
    lambda i: v,
    lambda x: check_const(v, x),
    len(v)
)

def frame(name, fields):
    class Frame(object):
        @classmethod
        def from_str(cls, s):
            kw = {}
            for f in fields:
                fs, s = s[:f.length], s[f.length:]
                fv = f.from_str(fs)
                if f.name:
                    kw[f.name] = fv
            return cls(**kw)
        def __init__(self, **kw):
            for n, v in kw.iteritems():
                setattr(self, n, v)
        def __str__(self):
            s = ''
            for f in fields:
                if f.name:
                    fs = f.to_str(getattr(self, f.name, f.default))
                else:
                    fs = f.to_str(None)
                s += fs
            return s
    return type(name, (Frame,), {})

Command = frame('Command', [
    Const('$'),
    Char('command', cmd_bytes),
    Const(','),
    Hex('uid', uid_bytes),
    Const(','),
    Char('hash', hash_bytes),
    Const(','),
    Char('mac', mac_bytes),
])

def signed_command(command='P', hash='00'*32, uid=0, token=''):
    """Returns a MACd Command instance."""
    data = str(Command(command=command, hash=hash, uid=uid, mac="aa"*32))
    data = ','.join(data.split(',')[:3])
    print data
    mac = hmac.HMAC(token, digestmod=hashlib.sha256)
    mac.update(data)
    return Command(command=command, hash=hash, uid=uid, mac=mac.hexdigest())
