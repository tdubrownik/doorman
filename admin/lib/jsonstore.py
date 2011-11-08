import json

from UserDict import IterableUserDict

class Storage(IterableUserDict):
    def __init__(self, encapsulation):
        self.encapsulation = encapsulation
        self.encapsulation.begin_transaction()
        try:
            stored = json.loads(self.encapsulation.data)
        except IOError:
            stored = {}
        IterableUserDict.__init__(self, stored)
    def __setitem__(self, k, v):
        IterableUserDict.__setitem__(self, k, v)
        self.sync()
    def __delitem__(self, k):
        IterableUserDict.__delitem__(self, k)
        self.sync()
    def __del__(self):
        self.sync()
    def sync(self):
        self.encapsulation.data = json.dumps(self.data, indent=4)
        self.encapsulation.end_transaction()
        self.encapsulation.begin_transaction()
