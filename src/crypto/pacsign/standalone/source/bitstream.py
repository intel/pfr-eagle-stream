##########################
#
# All about bitstream/data handling
#
# Base class is BITSTREAM_READER, which provide common API likely to be shared by different family (PAC_CARD is considered as family in this context)
#
#   PAC_CARD_READER is inherit from BITSTREAM_READER, which provide customized function on how to generate Block0 and Block1 according to PAC spec
#
#   In the future if more we need to support more family, just create a class inherit from base and customize this family flow
#       You need to define the database of this family in database.py
#       You also need to update openssl.py which curve is supported by this family
#
##########################
import common_util
import openssl
import database
import keychain

def get_family_reader(family, filename, openssl, insert, type, might_be_signed) :

    common_util.assert_in_error(family in database.FAMILY_LIST, "Supported family is %s, but found %s" % (database.FAMILY_LIST.keys(), family))
    if family == "PAC_CARD" :
        reader = PAC_DATA_READER(family, filename, openssl, insert, type, might_be_signed)
    else :
        common_util.assert_in_error("Family %s is not supported" % family)
    return reader

class BITSTREAM_READER :

    def __init__(self, family, filename, openssl, insert, type, might_be_signed) :
    
        common_util.assert_in_error(family in database.FAMILY_LIST, "Supported family is %s, but found %s" % (database.FAMILY_LIST.keys(), family))
    
        # Constant        
        self.family = family
        self.openssl = openssl
        self.database = database.FAMILY_LIST[family]
        self.rbf = None
        self.type = type
        if self.type != None :
            self.type = self.type.upper()
        if self.database.SUPPORTED_TYPES == None :
            common_util.assert_in_error(self.type == None, "Bitstream type is not supported, however type is specified")
        else :
            common_util.assert_in_error(self.type == None or self.type in self.database.SUPPORTED_TYPES, "Invalid type (%s). Supported types are %s" % (self.type, self.database.SUPPORTED_TYPES.keys()))
        
        # Read file
        file_data = common_util.BYTE_ARRAY("FILE", filename)
        common_util.assert_in_error(file_data.size() > 0 , "File size must be greater than zero")
        
        # Create descriptor and signature block
        if insert :
            # Append the data to be multiple of 128 Bytes aligned
            while file_data.size() % 128 :
                file_data.append_byte(0)
            self.generate_descriptor_and_signature_blocks(file_data.data)
            self.rbf.append_data(file_data.data)
            del file_data
        else :
            common_util.assert_in_error(file_data.size() > 1024, "Data size must be more than 1K Bytes, but found %d Bytes" % file_data.size())
            self.rbf = file_data
            self.verify_descriptor_and_signature_blocks(self.rbf.data[1024:], might_be_signed)
        
        # Size
        common_util.assert_in_error(self.rbf.size() > 1024 and self.rbf.size() % 128 == 0, "Data size must be more than 1K Bytes and multiple of 128 Bytes, but found %d Bytes" % self.rbf.size())
        
        # Check type again
        if self.database.SUPPORTED_TYPES != None :
            common_util.assert_in_error(self.type in self.database.SUPPORTED_TYPES, "Invalid type")
        
    def __del__(self) :
    
        self.clean()
            
    def __exit__(self) :
    
        self.clean()
            
    def clean(self) :
    
        if self.rbf != None :
            del self.rbf
            self.rbf = None
            
    def generate_descriptor_and_signature_blocks(self, raw_data) :
    
        # This is basic essential flow to generate decriptor and signature block
        self.generate_descriptor_block(raw_data)
        self.generate_signature_block()
        # Each family can further customize the flow
        self.customize_descriptor_and_signature_blocks()
        
    def generate_descriptor_block(self, raw_data) :
    
        common_util.assert_in_error(self.rbf == None, "Data should be None")
        common_util.assert_in_error(len(raw_data) % 128 == 0, "Content size should be multiple of 128 Bytes, but found %d Bytes" % len(raw_data))
        if self.database.SUPPORTED_TYPES != None :
            common_util.assert_in_error(self.type != None, "Content type must be specified")
        self.rbf = common_util.BYTE_ARRAY()
        
        # Offset 0x000 - 0x003 (magic num)
        self.rbf.append_dword(database.DESCRIPTOR_BLOCK_MAGIC_NUM)
        
        # Offset 0x004 - 0x007 (Size)
        self.rbf.append_dword(len(raw_data))
        
        # Offset 0x008 - 0x008 (Type)
        if self.database.SUPPORTED_TYPES != None :
            type = self.database.SUPPORTED_TYPES[self.type][0]
            self.rbf.append_byte(type)
        else :
            self.rbf.append_byte(0)
        
        # Offset 0x009 - 0x009 (cancellation certificate)
        # Todo: add support
        self.rbf.append_byte(0)
        
        # Offset 0x00A - 0x00F (reserved)
        while self.rbf.size() < 16 :
            self.rbf.append_byte(0)
            
        # Offset 0x010 - 0x02F (Sha256)
        self.generate_sha(raw_data, 32)
 
        # Offset 0x030 - 0x05F (Sha384)
        self.generate_sha(raw_data, 48)
        
        common_util.assert_in_error(self.rbf.size() == 96, "Expect data size to be 96 Bytes, but found %d Bytes" % self.rbf.size())
        
        # Offset 0x060 - 0x07F (reserved)
        while self.rbf.size() < 0x80 :
            self.rbf.append_byte(0)
            
        common_util.assert_in_error(self.rbf.size() == 128, "Block0 should be 128 Bytes, but found %d Bytes" % self.rbf.size())
        
    def generate_signature_block(self) :
    
        common_util.assert_in_error(self.rbf.size() == 128, "Block1 should start with 128 Bytes, but found %d Bytes" % self.rbf.size())
        
        # Offset 0x080 - 0x083 (magic num)
        self.rbf.append_dword(database.SIGNATURE_BLOCK_MAGIC_NUM)
        
        # Offset 0x084 - 0x3FF 
        for i in range(132, 1024) :
            self.rbf.append_byte(0)
            
        common_util.assert_in_error(self.rbf.size() == 1024, "Signature block should be %d Bytes, but found %d Bytes" % (1024 - 128, self.rbf.size() - 128))
        
    def customize_descriptor_and_signature_blocks(self) :
    
        common_util.assert_in_error(False, "Base class BITSTREAM_READER does not implement customize_descriptor_and_signature_blocks()")
        
    def verify_descriptor_and_signature_blocks(self, raw_data, might_be_signed) :
    
        # This is basic essential flow to generate decriptor and signature block
        self.verify_descriptor_block(raw_data)
        self.verify_signature_block(might_be_signed)
        # Each family can further customize the flow
        self.special_verify_descriptor_and_signature_blocks()
    
    def verify_descriptor_block(self, raw_data) :
    
        common_util.assert_in_error(self.rbf != None, "Data should not be None")
        common_util.assert_in_error(len(raw_data) % 128 == 0, "Content size should be multiple of 128 Bytes")
    
        # Offset 0x000 - 0x003 (magic num)
        temp = self.rbf.get_dword(0)
        common_util.assert_in_error(temp == database.DESCRIPTOR_BLOCK_MAGIC_NUM, "Block0 magic num should be 0x%08X, but found 0x%08X" % (database.DESCRIPTOR_BLOCK_MAGIC_NUM, temp))

        # Offset 0x004 - 0x007 (Size)
        temp = self.rbf.get_dword(4)
        common_util.assert_in_error(temp == len(raw_data), "Content size should be %d Bytes, but found %d Bytes in file" % (len(raw_data), temp))
        
        # Offset 0x008 - 0x008 (Type)
        type = self.rbf.data[8]
        if self.database.SUPPORTED_TYPES == None :
            common_util.assert_in_error(type == 0, "Family %s does not support bitstream type, bitstream type should be default to 0x00, but found 0x%02X" % (self.family, type))
            self.type = None
        else :
            self.type = self.database.get_type_from_enum(type)
            common_util.assert_in_error(self.type != None, "Type enum %d is not supported by family %s" % (type, self.family))
        
        # Offset 0x009 - 0x009 (cancellation certificate)
        # Todo: add support
        
        # Offset 0x00A - 0x00F
        # Checking done in special_verify_descriptor_and_signature_blocks()
            
        # Offset 0x010 - 0x02F (Sha256)
        self.check_block_hash(raw_data, self.rbf.data[0x10:0x30], 32)
        
        # Offset 0x030 - 0x05F (Sha384)
        self.check_block_hash(raw_data, self.rbf.data[0x30:0x60], 48)
        
        # Offset 0x060 - 0x07F
        # Checking done in special_verify_descriptor_and_signature_blocks()
        
    def verify_signature_block(self, might_be_signed) :
    
        common_util.assert_in_error(self.rbf != None, "Data should not be None")
        
        # Offset 0x080 - 0x083 (magic num)
        temp = self.rbf.get_dword(0x80)
        common_util.assert_in_error(temp == database.SIGNATURE_BLOCK_MAGIC_NUM, "Block1 magic num should be 0x%08X, but found 0x%08X" % (database.SIGNATURE_BLOCK_MAGIC_NUM, temp))
        
        # Offset 0x084 - 0x08F
        # Checking done in special_verify_descriptor_and_signature_blocks()
            
        # Offset 0x090 - end of signature (reserved)
        print_info = False
        if might_be_signed :
            must_be_zero = self.rbf.data[144] == 0
            print_info = True
        else :
            must_be_zero = True
            
        if must_be_zero :
            for i in range(144, 144 + self.database.SIGNATURE_MAX_SIZE) :
                common_util.assert_in_error(self.rbf.data[i] == 0, "Data at index %d should be reserved as 0 for signing, but found 0x%02X" % (i, self.rbf.data[i]))
            if print_info :
                common_util.print_info("File is not signed")
        else :
            qky = keychain.QKY_READER("", self.openssl, self.family, self.rbf.data[144:144 + self.database.SIGNATURE_MAX_SIZE], self.rbf.data[:128])
            common_util.assert_in_error(qky.complete, "Signing chain in bitstream is not complete")
            common_util.assert_in_error(self.is_permitted(qky.permission), "QKY with permission (0x%08X) should not be used to sign data type %s" % (qky.permission, self.type))
            common_util.print_info("File is signed")
            common_util.print_info("Fuse info: %s" % qky.fuse_info)
            del qky
        
    def special_verify_descriptor_and_signature_blocks(self) :
    
        common_util.assert_in_error(False, "Base class BITSTREAM_READER does not implement special_verify_descriptor_and_signature_blocks()")
        
    def generate_sha(self, data, hashsize) :
        
        common_util.assert_in_error(hashsize == 32 or hashsize == 48 or hashsize == 64, "Only support SHA256, SHA384 and SHA512, but found SHA%d" % (hashsize * 8))
        sha = self.openssl.get_byte_array_sha(hashsize, data)
        self.rbf.append_data(sha.data)
        del sha
            
    def check_block_hash(self, data, hashdata, hashsize) :

        common_util.assert_in_error(hashsize == 32 or hashsize == 48 or hashsize == 64, "Only support SHA256, SHA384 and SHA512, but found SHA%d" % (hashsize * 8))
        common_util.assert_in_error(len(hashdata) == hashsize, "Expect hash data size of signature block to be %d Bytes, but found %d Bytes" % (hashsize, len(hashdata)))
        sha = self.openssl.get_byte_array_sha(hashsize, data)
        hash = sha.get_standard_hex_string(0, hashsize);
        filehash = common_util.get_standard_hex_string(hashdata)
        del sha
        common_util.assert_in_error(hash == filehash, "HASH mismatch at signature block, expected hash is %s, but found %s in file" % (hash, filehash))
        
    def is_permitted(self, permission) :
    
        if self.database.SUPPORTED_TYPES == None :
            return True
        else :
            type_info = self.database.SUPPORTED_TYPES[self.type]
            if permission & type_info[1] :
                return True
            else :
                return False

class PAC_DATA_READER(BITSTREAM_READER) :

    def __init__(self, family, filename, openssl, insert, type, might_be_signed) :
    
        super(PAC_DATA_READER, self).__init__(family, filename, openssl, insert, type, might_be_signed)
        
    def customize_descriptor_and_signature_blocks(self) :
    
        # At this moment nothing needs to be customized
        pass
        
    def special_verify_descriptor_and_signature_blocks(self) :
    
        # Future family might use the reserved field for other stuff
        # Only PAC_DATA_READER will make sure it is ZERO
        # Offset 0x00A - 0x00F (reserved) 
        for i in range(0xA, 0x10) :
            common_util.assert_in_error(self.rbf.data[i] == 0, "Data at index %d should be reserved as 0, but found 0x%02X" % (i, self.rbf.data[i]))
            
        # Offset 0x060 - 0x07F (reserved)
        for i in range(0x60, 0x80) :
            common_util.assert_in_error(self.rbf.data[i] == 0, "Data at index %d should be reserved as 0, but found 0x%02X" % (i, self.rbf.data[i]))
        
        # Offset 0x084 - 0x08F (reserved)        
        for i in range(132, 144) :
            common_util.assert_in_error(self.rbf.data[i] == 0, "Data at index %d should be reserved as 0, but found 0x%02X" % (i, self.rbf.data[i]))
        