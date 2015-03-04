import options
import storage_encapsulation
from .classes import CsvStorage, JsonStorage

if options.storage_encrypt == True:
    encapsulation_class = storage_encapsulation.DESFileEncapsulation
else:
    encapsulation_class = storage_encapsulation.RawFileEncapsulation

if options.storage == 'json':
    encapsulation = encapsulation_class(options.json)
    storage = JsonStorage(encapsulation)
if options.storage == 'csv':
    encapsulation = encapsulation_class(options.csv)
    storage = CsvStorage(encapsulation)

nobody = (None, '-unknown-')

get_card = lambda h: storage.get(h, nobody)
cards_for_user = lambda name: map(lambda (k,v): k, 
    filter(lambda (k,(u,n)): n == name, storage.iteritems()))

def add_user(username, hash, uid):
    storage[hash] = (uid, username)

def del_card(hash):
    return storage.pop(hash, nobody)

def del_filter(f):
    cards = map(lambda (k,v): k, filter(f, storage.iteritems()))
    r = []
    for c in cards:
        r.append(storage.pop(c, nobody))
    return r

del_uid = lambda uid: del_filter(lambda (k, (u,n)): u == uid)
del_username = lambda name: del_filter(lambda (k, (u,n)): n == name)
