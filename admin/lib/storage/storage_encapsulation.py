# because admin.lib.password uses admin.options -_-
if __name__ == "__main__":
    import sys
    sys.path.append("d:\\Development\\Projects\\doorman\\admin")

import hashlib
import os
import tempfile

import lib.password

class RawFileEncapsulation(object):
    """
    Encapsulates data in a file. With style!
    
    The data is available in .data.
    """
    
    def __init__(self, filename):
        self.original_filename = filename
        self.original_checksum = None
        self.storage = None
    
    def _decode_data(self, data):
        """
        Override me.
        """
        return data
    
    def _encode_data(self, data):
        """
        Override me.
        """
        return data
    
    def _backup_file_path(self, original_path):
        """
        Override me if you care.
        """
        return original_path + "_OHSHITSOMETHINGWENTWRONG"
    
    def begin_transaction(self):
        """
        Creates a temporary file that we'll use before writing all.
        """
        
        with open(self.original_filename, "rb") as original:
            self.data = self._decode_data(original.read())
        self.original_checksum = hashlib.sha1(self.data).hexdigest()
    
    def end_transaction(self):
        """
        Verifies everything went well and commits changes to the file.
        """
        
        original = open(self.original_filename, "rb")
        verification_data = self._decode_data(original.read())
        verification_checksum = hashlib.sha1(verification_data).hexdigest()
        
        if verification_checksum != self.original_checksum:
            #TODO: Implement separate exception
            raise Exception("File changed since we last opened it!")
        
        # Let's make a backup of the old original... Because we're paranoid.
        # We're writing the encapsulated versiuon here so that it can be a 
        # drop-in replacement.
        backup_filename = self._backup_file_path(self.original_filename)
        with open(backup_filename, "wb") as backup:
            backup.write(verification_data)
        
        # BIG TODO: Fix the race condition that occurs here: the file can be
        # opened by another program here and it can be modified!
        # Oh well.
        
        original = open(self.original_filename, "wb")
        original.write(self._encode_data(self.data))
        
        # Remove the backup.
        os.remove(backup_filename)

class DESFileEncapsulation(RawFileEncapsulation):
    MAGIC = "it's a kind of magic!"
    def __init__(self, filename, magic_check=True):
        try:
            import Crypto
        except:
            raise Exception("PyCrypto not installed!")
            
        self.des_key = password.get_des_storage_key(filename)
        self.magic_check = magic_check
        super(DESFileEncapsulation, self).__init__(filename)
    
    def _decode_data(self, data):
        if data == "":
            print "Input file empty. Assuming actually empty file."
            return ""
        
        from Crypto.Cipher import DES
        des = DES.new(self.des_key, DES.MODE_CBC)
        data_padded = des.decrypt(data)
        
        padding_length = ord(data_padded[-1])
        data_unpadded = data_padded[:-padding_length]
        
        if self.magic_check and not data_unpadded.startswith(self.MAGIC):
            raise Exception("Bad magic! Did you mistype a key?")
        
        return data_unpadded[len(self.MAGIC):]
    
    def _encode_data(self, data):
        from Crypto.Cipher import DES
        des = DES.new(self.des_key, DES.MODE_CBC)
        
        if self.magic_check:
            data = self.MAGIC + data
        padding_length = 8 - len(data) % 8
        data_padded = data + chr(padding_length) * padding_length
        
        return des.encrypt(data_padded)

if __name__ == "__main__":
    r = DESFileEncapsulation("d:/dupa.txt")
    r.begin_transaction()
    
    try:
        n = int(r.data)
    except:
        n = 0
    
    print "ass! %i" % n
    r.data = str(n + 1)
    
    print "try to modify the assfile, see it fail!"
    raw_input()
    
    r.end_transaction()
