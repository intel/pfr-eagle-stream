#! /bin/bash

set -e

head -c 128 </dev/urandom > raw.dat

python PACSign.py --operation=MAKE_PRIVATE_PEM --curve=prime256v1 --no_passphrase private.pem
python PACSign.py --operation=MAKE_PUBLIC_PEM private.pem public.pem
python PACSign.py --operation=MAKE_ROOT public.pem public.qky
python PACSign.py --operation=MAKE_PRIVATE_PEM --curve=prime256v1 --no_passphrase private2.pem
python PACSign.py --operation=MAKE_PUBLIC_PEM private2.pem public2.pem
python PACSign.py --operation=append_key --previous_pem=private.pem --previous_qky=public.qky public2.pem public2.qky --permission=2 --cancel=0 
python PACSign.py --operation=INSERT_DATA_AND_SIGN --type=BMC_FW --qky=public2.qky --pem=private2.pem raw.dat signed.dat

