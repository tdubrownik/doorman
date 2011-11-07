import json

from UserDict import IterableUserDict

class Storage(IterableUserDict):
    def __init__(self, name, **kw):
        self.filename = name
        try:
            stored = json.load(open(name))
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
        json.dump(self.data, open(self.filename, 'w'), indent=4)
