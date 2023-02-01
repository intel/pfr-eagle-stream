#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/ec.h>
#include <openssl/obj_mac.h>
#include <openssl/bn.h>
#include <openssl/objects.h>
#include <openssl/ecdsa.h>
#include <openssl/evp.h>
#include <random>
#include <vector>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <memory>

using namespace std;

std::string print_hex_string(const std::vector<unsigned char> data)
{
    std::string ret;
    BIGNUM* bn = BN_bin2bn(data.data(), data.size(), nullptr);

    ret = BN_bn2hex(bn);

    BN_free(bn);

    return ret;
}

void print_hex_string_as_c_array(const char *hex_str, int num_bytes)
{    
    std::string c_array_str("{");
    std::string hex_string(hex_str);
    // Each byte can fit 2 hex characters
    for (int i=0; i<num_bytes*2; i += 8)
    {
        c_array_str.append("0x");
        c_array_str.append(hex_string.substr(i, 8));
        if (i + 8 != num_bytes*2)
            c_array_str.append(", ");
    }
    c_array_str.append("};");
    cout << "\tunsigned char arr[] = " << c_array_str << endl;
}

std::string calculate_sha(EVP_MD_CTX* ctx) 
{
    unsigned int md_len;
    unsigned char md_value[256];
    std::string ret;

    ret.reserve(2*256);

    // Finalize the digest
    EVP_DigestFinal_ex(ctx, md_value, &md_len);

    for(int i = 0; i < md_len; i++) 
    {
        char buf[16];
        snprintf(buf, 16, "%02X", md_value[i]);
        ret += buf;
    }

    return ret;
}


