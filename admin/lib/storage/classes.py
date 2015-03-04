from sys import stderr
from collections import MutableMapping
from StringIO import StringIO

import csv, json

class Storage(MutableMapping):
    def __init__(self, encapsulation):
        self.encapsulation = encapsulation
        self.encapsulation.begin_transaction()
        self.data = self.decode(encapsulation.data)
    def sync(self):
        self.encapsulation.data = self.encode(self.data)
        self.encapsulation.end_transaction()
        
        self.encapsulation.begin_transaction()
    def __setitem__(self, k, v):
        self.data[k] = v
        self.sync()
    def __delitem__(self, k):
        del self.data[k]
        self.sync()
    def __getitem__(self, k):
        return self.data[k]
    def __len__(self, k):
        return len(self.data)
    def __iter__(self):
        return iter(self.data)

class CsvStorage(Storage):
    def decode(self, text):
        try:
            stored = {x[0]: [x[1], x[2]] 
                for x in csv.reader(StringIO(text))}
        except IOError as e:
            print >>stderr, e
            stored = {}
        return stored
    def encode(self, data):
        f = StringIO()
        csv.writer(f).writerows(
            [c, u, name] for c, (u, name) in data.iteritems())
        return f.getvalue()

class JsonStorage(Storage):
    def decode(self, text):
        try:
            stored = json.loads(self.encapsulation.data)
        except IOError as e:
            print >>stderr, e
            stored = {}
        return stored
    def encode(self, data):
        return json.dumps(data, indent=4)
        
