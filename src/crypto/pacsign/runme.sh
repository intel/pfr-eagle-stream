#! /bin/bash

set -e

head -c 128 </dev/urandom > raw.dat

pgm_py pac_card_sign --operation=MAKE_PRIVATE_PEM --curve=prime256v1 --no_passphrase private.pem
pgm_py pac_card_sign --operation=MAKE_PUBLIC_PEM private.pem public.pem
pgm_py pac_card_sign --operation=MAKE_ROOT public.pem public.qky
pgm_py pac_card_sign --operation=MAKE_PRIVATE_PEM --curve=prime256v1 --no_passphrase private2.pem
pgm_py pac_card_sign --operation=MAKE_PUBLIC_PEM private2.pem public2.pem
pgm_py pac_card_sign --operation=append_key --previous_pem=private.pem --previous_qky=public.qky public2.pem public2.qky --permission=1 --cancel=0
pgm_py pac_card_sign --operation=INSERT_DATA_AND_SIGN --type=BBS --qky=public2.qky --pem=private2.pem raw.dat signed.dat

