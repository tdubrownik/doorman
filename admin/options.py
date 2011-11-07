url = '/dev/ttyACM0'
serial = dict(
    baudrate = 19200,
)
shelf = './base'
json = './cards.json'
csv = './cards.csv'

storage = 'csv'
#whether the storage method should be encrypted
storage_encrypt = True

mid_bytes = 8
uid_bytes = 4
cmd_bytes = 1
empty_uid = 0
empty_mid = 0

pin_bytes = 4
token_bytes = 8

init_sleep = 3
