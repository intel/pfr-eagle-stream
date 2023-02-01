#! /bin/bash

set -e

# Generate random file
head -c 1048576 </dev/urandom >myfile

# Curve parameters
openssl ecparam -name secp384r1 -text -param_enc explicit -noout
#Field Type: prime-field
#Prime:
#    00:ff:ff:ff:ff:ff:ff:ff:ff:ff:ff:ff:ff:ff:ff:
#    ff:ff:ff:ff:ff:ff:ff:ff:ff:ff:ff:ff:ff:ff:ff:
#    ff:ff:fe:ff:ff:ff:ff:00:00:00:00:00:00:00:00:
#    ff:ff:ff:ff
#A:   
#    00:ff:ff:ff:ff:ff:ff:ff:ff:ff:ff:ff:ff:ff:ff:
#    ff:ff:ff:ff:ff:ff:ff:ff:ff:ff:ff:ff:ff:ff:ff:
#    ff:ff:fe:ff:ff:ff:ff:00:00:00:00:00:00:00:00:
#    ff:ff:ff:fc
#B:   
#    00:b3:31:2f:a7:e2:3e:e7:e4:98:8e:05:6b:e3:f8:
#    2d:19:18:1d:9c:6e:fe:81:41:12:03:14:08:8f:50:
#    13:87:5a:c6:56:39:8d:8a:2e:d1:9d:2a:85:c8:ed:
#    d3:ec:2a:ef
#Generator (uncompressed):
#    04:aa:87:ca:22:be:8b:05:37:8e:b1:c7:1e:f3:20:
#    ad:74:6e:1d:3b:62:8b:a7:9b:98:59:f7:41:e0:82:
#    54:2a:38:55:02:f2:5d:bf:55:29:6c:3a:54:5e:38:
#    72:76:0a:b7:36:17:de:4a:96:26:2c:6f:5d:9e:98:
#    bf:92:92:dc:29:f8:f4:1d:bd:28:9a:14:7c:e9:da:
#    31:13:b5:f0:b8:c0:0a:60:b1:ce:1d:7e:81:9d:7a:
#    43:1d:7c:90:ea:0e:5f
#Order: 
#    00:ff:ff:ff:ff:ff:ff:ff:ff:ff:ff:ff:ff:ff:ff:
#    ff:ff:ff:ff:ff:ff:ff:ff:ff:ff:c7:63:4d:81:f4:
#    37:2d:df:58:1a:0d:b2:48:b0:a7:7a:ec:ec:19:6a:
#    cc:c5:29:73
#Cofactor:  1 (0x1)
#Seed:
#    a3:35:92:6a:a3:19:a2:7a:1d:00:89:6a:67:73:a4:
#    82:7a:cd:ac:73

# Generate private key
openssl ecparam -name secp384r1 -genkey -noout -out secp384r1-privkey.pem

# Create public key from private key
openssl ec -in secp384r1-privkey.pem  -pubout -out secp384r1-pubkey.pem.pem

# Dump the key in text form
openssl ec -in secp384r1-privkey.pem  -text -noout
#read EC key
#Private-Key: (384 bit)
#priv:
#    5e:15:24:57:4f:6b:62:8d:58:8b:62:11:75:b8:85:
#    b5:68:c7:10:11:c0:91:0c:1e:e6:77:2d:c3:ff:81:
#    e9:5f:d5:ac:f4:0b:59:5b:a2:80:dc:e2:0e:4f:0d:
#    57:c2:b0
#pub: 
#    04:34:e6:58:fa:2e:2d:9f:ae:26:37:4f:8e:ca:5c:
#    2c:a8:ec:fb:be:1e:84:48:a6:62:02:0f:75:73:5f:
#    91:7c:a5:7a:3d:4f:27:cf:d9:c2:ae:bb:7d:dc:28:
#    bb:dc:52:3f:e9:32:28:3a:33:a2:8f:00:6c:89:65:
#    ef:5b:5f:5d:ba:05:7f:c1:f7:9b:ce:9b:2b:87:34:
#    ba:98:8b:08:5a:e7:17:2e:e8:05:91:d4:8b:4a:7d:
#    0c:af:7c:fe:30:1a:1d
#ASN1 OID: secp384r1
#NIST CURVE: P-384

# Generate hash
openssl dgst -sha384 myfile

# Generate signature
openssl dgst -sha384 -sign secp384r1-privkey.pem -out myfile.sig myfile
#EC-SHA384(myfile)= 3045022100bace5490033586c213c7d701a26e1f90599b2fad026d32df84692722c5682f3b02202538a7f2f45f02f9f92bb03d8bd9d2e0187cad80a81d3e667b132acb1ea9fa35

# Decode the signature
openssl asn1parse -inform DER -in myfile.sig
#    0:d=0  hl=2 l= 100 cons: SEQUENCE          
#    2:d=1  hl=2 l=  48 prim: INTEGER           :056A5731CD472178B91F96F6D6B4799D42055F8F5CD7788ECE38605F660CB2D7F16ABB91A9EA3E605D0E74888A4691A7
#   52:d=1  hl=2 l=  48 prim: INTEGER           :73964E227C095D82D349AC059305AD0BE795BFAA0E238BFF31A2C31D94A5ADCBF3E11C53D8A8BD4765BB51AE2CED0F93

