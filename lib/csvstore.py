from UserDict import IterableUserDict
import csv

class Storage(IterableUserDict):
    def __init__(self, name):
        self.filename = name
        try:
            stored = {x[0]: [x[1], x[2]] 
                for x in csv.reader(open(name))}
        except IOError:
            stored = {}
        IterableUserDict.__init__(self, stored)
    def sync(self):
        csv.writer(open(self.filename, 'w')).writerows(
            [c, u, name] for c, (u, name) in self.data.iteritems())
    def __del__(self):
        self.sync()
    def __setitem__(self, k, v):
        IterableUserDict.__setitem__(self, k, v)
        self.sync()
    def __delitem__(self, k):
        IterableUserDict.__delitem__(self, k)
        self.sync()
