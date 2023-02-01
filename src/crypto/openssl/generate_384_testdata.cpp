#include <stdio.h>
#include <stdlib.h>
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

using namespace std;


#define CURVE_NID NID_secp384r1

#define BLOCK_SIZE_BYTES 1024/8

#define NUM_1_BLOCK_PATTERNS 1
#define NUM_2_BLOCK_PATTERNS 1
#define NUM_3_BLOCK_PATTERNS 1
#define NUM_4_BLOCK_PATTERNS 1
#define NUM_5_BLOCK_PATTERNS 1
#define NUM_6_BLOCK_PATTERNS 1


std::string print_hex_string(const std::vector<unsigned char> data)
{
    std::string ret;
    BIGNUM* bn = BN_bin2bn(data.data(), data.size(), nullptr);

    ret = BN_bn2hex(bn);

    BN_free(bn);

    return ret;
}

std::string calculate_sha(EVP_MD_CTX* ctx) 
{
    unsigned int md_len;
    unsigned char md_value[384];
    std::string ret;

    ret.reserve(2*384);

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

EC_KEY* generate_key()
{
    // The EC key pair
    EC_KEY* eckey = nullptr;

    // Initialize BigNum context buffer
    BN_CTX* ctx = BN_CTX_new();

    // Initialize the key using the curve name
    cout << "Generating curve" << endl;
    eckey = EC_KEY_new_by_curve_name(CURVE_NID);

    cout << "Using curve " << OBJ_nid2ln(CURVE_NID) << endl;

    // Get the parameters from the curve
    const EC_GROUP* group = EC_KEY_get0_group(eckey);

    // Generate the private key
    cout << "Generating private key" << endl;
    EC_KEY_generate_key(eckey);

    // Print the private key
    cout << "Private Key:" << endl;
    const BIGNUM* eckey_privkey = EC_KEY_get0_private_key(eckey);
    const char* privkey_hex_str = BN_bn2hex(eckey_privkey);
    cout << "\t\t384'h" << privkey_hex_str << endl;

    // Print the public key:
    char* pubkey_cx_cy_str_ptr = EC_POINT_point2hex(group, EC_KEY_get0_public_key(eckey), POINT_CONVERSION_UNCOMPRESSED, ctx);
    char* pubkey_cx_cy_str = pubkey_cx_cy_str_ptr;

    cout << "Pubkey:" << endl;
    cout << "\tCX : 384'h";
    for (int i = 2; i < (2 + 96); i++) // 1 byte 0x40, 96 bytes for X coordinate, 96 bytes for Y coordinate
    {
        printf("%c", pubkey_cx_cy_str[i]);
    }
    cout << endl;

    cout << "\tCY : 384'h";
    for (int i = 2 + 96; i < (2 + 96 + 96); i++) // 1 byte 0x40, 96 bytes for X coordinate, 96 bytes for Y coordinate
    {
        printf("%c", pubkey_cx_cy_str[i]);
    }
    cout << endl;

    BN_CTX_free(ctx);
    free(pubkey_cx_cy_str_ptr);

    return eckey;
}

void create_signature(const std::string& digest, EC_KEY* eckey)
{
    // Convert the digest to a bignum then print
    BIGNUM* hash_bn = BN_new();
    BN_hex2bn(&hash_bn, digest.c_str());
    unsigned char* hash_bytes = (unsigned char*)malloc(BN_num_bytes(hash_bn));
    BN_bn2bin(hash_bn, hash_bytes);
    cout << "Hash:" << endl << "\tE :  384'h" << BN_bn2hex(hash_bn) << endl;

    const ECDSA_SIG* signature = ECDSA_do_sign(hash_bytes, BN_num_bytes(hash_bn), eckey);
    const char* signature_r_hex_str = BN_bn2hex(signature->r);
    const char* signature_s_hex_str = BN_bn2hex(signature->s);

    cout << "Signature:"<< endl;
    cout << "\tR :  384'h" << signature_r_hex_str << endl;
    cout << "\tS :  384'h" << signature_s_hex_str << endl;

    free(hash_bytes);
    BN_free(hash_bn);
}


int main()
{
    const std::vector<int> patterns_per_block = {NUM_1_BLOCK_PATTERNS, NUM_2_BLOCK_PATTERNS, NUM_3_BLOCK_PATTERNS, NUM_4_BLOCK_PATTERNS, NUM_5_BLOCK_PATTERNS, NUM_6_BLOCK_PATTERNS};

    printf("ECDSA384/SHA384 Test Generator\n");

    // OpenSSL Initialization
    OpenSSL_add_all_algorithms();

    // Initialize the random number generator
    std::random_device rand_device;
    std::mt19937 rand_generator(rand_device());
    std::uniform_int_distribution<> rand_dist(0, 255);

    std::vector<unsigned char> test_data(BLOCK_SIZE_BYTES);

    for (int block_index = 0; block_index < patterns_per_block.size(); block_index++) {
        int num_blocks = block_index + 1;
        cout << std::string(80, '-') << endl;
        cout << "INFO: Preparing to generate " << patterns_per_block[block_index] << " patterns of " << num_blocks << " blocks" << endl;

        // Generate the number of patters for this block size
        for (int pattern = 0; pattern < patterns_per_block[block_index]; pattern++)
        {
            // Initialize
            EVP_MD_CTX* mdctx = EVP_MD_CTX_create();
            const EVP_MD* md = EVP_sha384();

            cout << std::string(80, '-') << endl;
            printf("Generating pattern %d of %d bytes (%d bits)\n", pattern, num_blocks*BLOCK_SIZE_BYTES, num_blocks*BLOCK_SIZE_BYTES*8);

            // Initialize the SHA
            EVP_DigestInit_ex(mdctx, md, NULL);

            // Iterate over all blocks to generate the test data and update the SHA
            for (int block = 0; block < num_blocks; block++)
            {
                // Generate the random data for the block
                for (unsigned char& dbyte : test_data)
                {
                    dbyte = static_cast<unsigned char>(0xFF & rand_dist(rand_generator));
                }

                cout << "Test Data block " << block << ": " << (BLOCK_SIZE_BYTES*8) << "'h" << print_hex_string(test_data) << endl;

                // Create the SHA on the block
                EVP_DigestUpdate(mdctx, test_data.data(), BLOCK_SIZE_BYTES);

            }

            // Calculate the SHA on the data and print it
            const std::string digest = calculate_sha(mdctx);
            cout << "Digest : 384'h" << digest << endl;

            // Generate the private and public key, then print it
            EC_KEY* eckey = generate_key();

            // Generate the signature of the hash
            create_signature(digest, eckey);

            EVP_MD_CTX_destroy(mdctx);
        }
    }

    cout << "Done!" << endl;

    EVP_cleanup();
    return 0;
}




