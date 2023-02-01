import os
print ("Generate two pairs of private/public key")
assert os.system("python PACSign.py --operation=make_private_pem --curve=secp384r1 --no_passphrase test/pri1.pem") == 0
assert os.system("python PACSign.py --operation=make_private_pem --curve=secp384r1 --no_passphrase test/pri2.pem") == 0
assert os.system("python PACSign.py --operation=make_public_pem test/pri1.pem test/pub1.pem") == 0
assert os.system("python PACSign.py --operation=make_public_pem test/pri2.pem test/pub2.pem") == 0
print ("Generate root keychain")
assert os.system("python PACSign.py --operation=make_root test/pub1.pem test/root.qky") == 0
print ("Append new key to root keychain to generate new keychain")
assert os.system("python PACSign.py --operation=append_key --permission=-1 --cancel=0 --previous_qky=test/root.qky --previous_pem=test/pri1.pem test/pub2.pem test/key.qky") == 0
print ("Insert Block0/Block1 into raw data and sign")
assert os.system("python PACSign.py --operation=insert_data_and_sign --type=BBS --qky=test/key.qky --pem=test/pri2.pem test/data.bin test/data1sign.bin") == 0
print ("Insert Block0/Block1 into raw data, the output is unsigned data")
assert os.system("python PACSign.py --operation=insert_data --type=BMC_FW test/data.bin test/unsigned_data.bin") == 0
print ("Sign the unsigned data")
assert os.system("python PACSign.py --operation=sign --qky=test/key.qky --pem=test/pri2.pem test/unsigned_data.bin test/data2sign.bin") == 0
print ("Read fuse")
assert os.system("python PACSign.py --operation=fuse_info test/root.qky test/root.txt") == 0
assert os.system("python PACSign.py --operation=fuse_info test/key.qky test/key.txt") == 0
print ("Check file integrity")
assert os.system("python PACSign.py --operation=check_integrity test/unsigned_data.bin > test/unsigned_data.bin.txt") == 0
assert os.system("python PACSign.py --operation=check_integrity test/data1sign.bin > test/data1sign.bin.txt") == 0
assert os.system("python PACSign.py --operation=check_integrity test/data2sign.bin > test/data2sign.bin.txt") == 0
print ("Misc")
assert os.system("python PACSign.py --help > test/help.txt") == 0
assert os.system("python PACSign.py --help --operation=sign  > test/ohelp.txt") == 0