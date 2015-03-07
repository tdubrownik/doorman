url = '/dev/ttyUSB6'
serial = dict(
    baudrate = 19200,
    timeout = 60,
)
shelf = './base'
json = './cards.json'
csv = './cards.csv'

storage = 'csv'
#whether the storage method should be encrypted
storage_encrypt = False

uid_bytes = 4
cmd_bytes = 1
empty_uid = 0
empty_hash = '0' * 64
empty_mid = 0

hash_bytes = 64

mac_bytes = 64

init_sleep = 5
