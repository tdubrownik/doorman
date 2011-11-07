import hashlib
import getpass

import options

def get_token():
    password = getpass.getpass('Password:')
    return hashlib.sha1(password).hexdigest()[:options.token_bytes]

def get_pin():
    pin = getpass.getpass('PIN:')
    return [ord(i) - ord('0') for i in pin]
