from proto import Proto
from options import *
from command import signed_command

def scan(token, url=None, proto=None):
    proto = proto or Proto(url)
    proto.send(signed_command(command='G', uid=0, hash=empty_hash, token=token))
    return proto.recv()

def add(token, hash, uid=0, url=None, proto=None):
    proto = proto or Proto(url)
    proto.send(signed_command(command='A', hash=hash, uid=uid, token=token))
    return proto.recv()

def revoke_uid(token, uid, url=None, proto=None):
    proto = proto or Proto(url)
    proto.send(signed_command(command='R', hash=empty_hash, uid=uid, token=token))
    return proto.recv()

def revoke_hash(token, hash, url=None, proto=None):
    proto = proto or Proto(url)
    proto.send(signed_command(command='R', hash=hash, uid=0, token=token))
    return proto.recv()
