from proto import Proto
from command import Command

def scan(token, url=None, proto=None):
    proto = proto or Proto(url)
    proto.send(Command(command='G', mid=0, uid=0, token=token))
    return proto.recv()

def add(token, mid, pin, uid=0, url=None, proto=None):
    proto = proto or Proto(url)
    proto.send(Command(command='A', mid=mid, uid=uid, 
        pin=''.join(chr(i + ord('0')) for i in pin), token=token))
    return proto.recv()

def revoke_uid(token, uid, url=None, proto=None):
    proto = proto or Proto(url)
    proto.send(Command(command='R', mid=0, uid=uid, token=token))
    return proto.recv()

def revoke_mid(token, mid, url=None, proto=None):
    proto = proto or Proto(url)
    proto.send(Command(command='R', mid=mid, uid=0, token=token))
    return proto.recv()
