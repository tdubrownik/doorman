import hashlib
import os
import tempfile

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
    
    def begin_transaction(self, storage=None):
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