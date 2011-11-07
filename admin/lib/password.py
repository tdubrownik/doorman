import hashlib
import getpass

import options

def get_des_storage_key(filename):
    password = getpass.getpass("DES Storage key (%s):" % filename)
    return hashlib.sha256(password).digest()[:8]

def get_token():
    password = getpass.getpass('Password:')
    return hashlib.sha1(password).hexdigest()[:options.token_bytes]

def get_pin():
    pin = getpass.getpass('PIN:')
    return [ord(i) - ord('0') for i in pin]
