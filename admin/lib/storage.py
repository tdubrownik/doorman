import shelve

import csvstore
import jsonstore
import options
import storage_encapsulation

if options.storage_encrypt == True:
    encapsulation_class = storage_encapsulation.DESFileEncapsulation
else:
    encapsulation_class = storage_encapsulation.RawFileEncapsulation

if options.storage == 'shelve':
    if options.storage_encrypt == True:
        raise NotImplementedError("Who the fuck uses Shelve anyway.")
    storage = shelve.open(options.shelf)
if options.storage == 'json':
    encapsulation = encapsulation_class(options.json)
    storage = jsonstore.Storage(encapsulation)
if options.storage == 'csv':
    encapsulation = encapsulation_class(options.csv)
    storage = csvstore.Storage(encapsulation)

nobody = (None, '-unknown-')

to_hex = lambda v: '%x' % v
from_hex = lambda v: int(v, 16)
dehex = lambda s: int(s.translate(None, 'xXlL'), 16)

get_card = lambda mid: storage.get(to_hex(mid), nobody)
cards_for_user = lambda name: map(lambda (k,v): from_hex(k), 
    filter(lambda (k,(u,n)): n == name, storage.iteritems()))

def add_user(username, mid, uid):
    storage[to_hex(mid)] = (uid, username)

def del_card(mid):
    return storage.pop(to_hex(mid), nobody)

def del_filter(f):
    cards = map(lambda (k,v): k, filter(f, storage.iteritems()))
    r = []
    for c in cards:
        r.append(storage.pop(c, nobody))
    return r

del_uid = lambda uid: del_filter(lambda (k, (u,n)): u == uid)
del_username = lambda name: del_filter(lambda (k, (u,n)): n == name)
