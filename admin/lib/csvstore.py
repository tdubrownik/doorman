from sys import stderr
from UserDict import IterableUserDict
from StringIO import StringIO

import csv

class Storage(IterableUserDict):
    def __init__(self, encapsulation):
        self.encapsulation = encapsulation
        self.encapsulation.begin_transaction()
        try:
            stored = {x[0]: [x[1], x[2]] 
                for x in csv.reader(StringIO(self.encapsulation.data))}
        except IOError as e:
            print >>stderr, e
            stored = {}
        IterableUserDict.__init__(self, stored)
    def sync(self):
        f = StringIO()
        csv.writer(f).writerows(
            [c, u, name] for c, (u, name) in self.data.iteritems())
        
        self.encapsulation.data = f.getvalue()
        self.encapsulation.end_transaction()
        f.close()
        
        self.encapsulation.begin_transaction()
    def __del__(self):
        #TODO: fix this shit
        #self.sync()
        pass
    def __setitem__(self, k, v):
        IterableUserDict.__setitem__(self, k, v)
        self.sync()
    def __delitem__(self, k):
        IterableUserDict.__delitem__(self, k)
        self.sync()
