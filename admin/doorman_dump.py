#!/usr/bin/env python
from lib.proto import Proto
from lib.command import Command
from lib.password import get_token
import options

token = get_token()
p = Proto(url=options.url)
p.send(Command(command='P', token=token))
while True:
    print p.fd.readline()
