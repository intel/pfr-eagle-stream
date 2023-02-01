import common_util

class FAMILY_DATABASE(object) :
    
    def __init__(self, supported_types, min_code_signing_key_entries, max_code_signing_key_entries, signature_max_size, supported_cancels) :
    
        self.SUPPORTED_TYPES = supported_types  # You can set the types to None, if this family has only one type data
        self.MIN_CODE_SIGNING_KEY_ENTRIES = min_code_signing_key_entries
        self.MAX_CODE_SIGNING_KEY_ENTRIES = max_code_signing_key_entries
        self.SIGNATURE_MAX_SIZE = signature_max_size
        self.SUPPORTED_CANCELS = supported_cancels # You can set cancels to None, if it does not support cancel
        common_util.assert_in_error(self.MAX_CODE_SIGNING_KEY_ENTRIES >= self.MIN_CODE_SIGNING_KEY_ENTRIES, "Impossible Code Signing Key entry count [min: %d, max: %d]" % (self.MIN_CODE_SIGNING_KEY_ENTRIES, self.MAX_CODE_SIGNING_KEY_ENTRIES))
        common_util.assert_in_error(self.SIGNATURE_MAX_SIZE > 0 and self.SIGNATURE_MAX_SIZE <= 880 and (self.SIGNATURE_MAX_SIZE % 4) == 0, "Maximum signature reserved field should greater than zero, less than 880 Bytes and multiple of 4 Bytes, but found %d" % self.SIGNATURE_MAX_SIZE)
        
    def get_type_from_enum(self, enum) :
    
        type = None
        for key in self.SUPPORTED_TYPES :
            if self.SUPPORTED_TYPES[key][0] == enum :
                type = key
                break
        return type

# For each supported family, the content type is very likely to be different as well as supported signing chain entry
FAMILY_LIST =   {
                    "PAC_CARD"      :   FAMILY_DATABASE({ "BBS" : [0, 1], "BMC_FW" : [1, 2], "GBS" : [2, 4] }, 1, 1, 880, [i for i in range(0, 128)])
                }

# As long as we are still using the same crypto IP/FW the constant here should not change
# Define it here so that signer + keychain can access same data
DESCRIPTOR_BLOCK_MAGIC_NUM = 0xB6EAFD19
SIGNATURE_BLOCK_MAGIC_NUM = 0xF27F28D7
ROOT_ENTRY_MAGIC_NUM = 0xA757A046
CODE_SIGNING_KEY_ENTRY_MAGIC_NUM = 0x14711C2F
BLOCK0_MAGIC_NUM = 0x15364367
             