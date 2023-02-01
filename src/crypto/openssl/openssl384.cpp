#include <stdio.h>
#include <stdlib.h>
#include <openssl/ec.h>
#include <openssl/obj_mac.h>
#include <openssl/bn.h>
#include <openssl/objects.h>
#include <openssl/ecdsa.h>
#include <openssl/evp.h>


#define CURVE_NID NID_secp384r1

int main()
{
    // OpenSSL Initialization
    OpenSSL_add_all_algorithms();

    // Temp string
    char* temp_cc = nullptr;

    // The EC key pair
    EC_KEY* eckey = nullptr;

    // Initialize BigNum context buffer
    BN_CTX* ctx = BN_CTX_new();

    // Initialize the key using the curve name
    printf("Generating curve\n");
    eckey = EC_KEY_new_by_curve_name(CURVE_NID);

    const char* curve_name_str = OBJ_nid2ln(CURVE_NID);
    printf("Using curve %s\n", curve_name_str);

    // Get the parameters from the curve
    const EC_GROUP* group = EC_KEY_get0_group(eckey);
    const EC_POINT* curve_generator = EC_GROUP_get0_generator(group);
    BIGNUM curve_order;
    BN_init(&curve_order);
    EC_GROUP_get_order(group, &curve_order, ctx);
    BIGNUM curve_cofactor;
    BN_init(&curve_cofactor);
    EC_GROUP_get_cofactor(group, &curve_cofactor, ctx);
    BIGNUM curve_prime;
    BN_init(&curve_prime);
    BIGNUM curve_a;
    BN_init(&curve_a);
    BIGNUM curve_b;
    BN_init(&curve_b);
    EC_GROUP_get_curve_GFp(group, &curve_prime, &curve_a, &curve_b, ctx);

    // Print the curve parameters
    temp_cc = EC_POINT_point2hex(group, curve_generator, POINT_CONVERSION_UNCOMPRESSED, ctx);
    printf("EC %s generator\n", curve_name_str);
    printf("\tBX : ");
    for (int i = 2; i < (2 + 96); i++) // 1 byte 0x40, 96 bytes for X coordinate, 96 bytes for Y coordinate
    {
        printf("%c", temp_cc[i]);
    }
    printf("\n");
    printf("\tBY : ");
    for (int i = 2 + 96; i < (2 + 96 + 96); i++) // 1 byte 0x40, 96 bytes for X coordinate, 96 bytes for Y coordinate
    {
        printf("%c", temp_cc[i]);
    }
    printf("\n");
    temp_cc = BN_bn2hex(&curve_order);
    printf("EC %s curve_order\n\tN :  %s\n", curve_name_str, temp_cc);
    temp_cc = BN_bn2hex(&curve_cofactor);
    printf("EC %s curve_cofactor\n\t     %s\n", curve_name_str, temp_cc);
    temp_cc = BN_bn2hex(&curve_prime);
    printf("EC %s curve_prime\n\tP :  %s\n", curve_name_str, temp_cc);
    temp_cc = BN_bn2hex(&curve_a);
    printf("EC %s curve_a\n\tA :  %s\n", curve_name_str, temp_cc);
    temp_cc = BN_bn2hex(&curve_b);
    printf("EC %s curve_b\n\t     %s\n", curve_name_str, temp_cc);


#ifdef SPECIFY_KEY
{
    // Create key based on hex string

    printf("Using specified private key\n");
    //const char* specified_privkey_hex_str = "5e1524574f6b628d588b621175b885b568c71011c0910c1ee6772dc3ff81e95fd5acf40b595ba280dce20e4f0d57c2b0";
    const char* specified_privkey_hex_str = "a4ebcae5a665983493ab3e626085a24c104311a761b5a8fdac052ed1f111a5c44f76f45659d2d111a61b5fdd97583480";

//     BN_hex2bn(&res,"3D79F601620A6D05DB7FED883AB8BCD08A9101B166BC60166869DA5FC08D936E");
    //BN_hex2bn(&res,"18E14A7B6A307F426A94F8114701E7C8E774E7F9A47E2C2035DB29A206321725");
    // Santosh BN_hex2bn(&res, "a4ebcae5a665983493ab3e626085a24c104311a761b5a8fdac052ed1f111a5c44f76f45659d2d111a61b5fdd97583480");

    printf("Given private key string = %s\n", specified_privkey_hex_str);

    // Convert the hex string to a BIGNUM
    BIGNUM* eckey_privkey_bn = BN_new();
    BN_hex2bn(&eckey_privkey_bn, specified_privkey_hex_str);

    // Set the private key into the eckey
    EC_KEY_set_private_key(eckey, eckey_privkey_bn);

    // Create the public key
    EC_POINT* pub_key = EC_POINT_new(group);
    EC_POINT_mul(group, pub_key, eckey_privkey_bn, NULL, NULL, ctx);

    // Set the public key in the EC key
    EC_KEY_set_public_key(eckey, pub_key);

    BN_free(eckey_privkey_bn);
}
#else

    // Generate the private key
    printf("Generating private key\n");
    EC_KEY_generate_key(eckey);


#endif
    // Print the private key
    printf("Private Key:\n");
    const BIGNUM* eckey_privkey = EC_KEY_get0_private_key(eckey);
    const char* privkey_hex_str = BN_bn2hex(eckey_privkey);
    printf("\t     %s\n", privkey_hex_str);

    // Print the public key:
    char* cc = EC_POINT_point2hex(group, EC_KEY_get0_public_key(eckey), POINT_CONVERSION_UNCOMPRESSED, ctx);
    char* c = cc;

    printf("Pubkey:\n");
    printf("\tCX : ");
    for (int i = 2; i < (2 + 96); i++) // 1 byte 0x40, 96 bytes for X coordinate, 96 bytes for Y coordinate
    {
        printf("%c", c[i]);
    }
    printf("\n");

    printf("\tCY : ");
    for (int i = 2 + 96; i < (2 + 96 + 96); i++) // 1 byte 0x40, 96 bytes for X coordinate, 96 bytes for Y coordinate
    {
        printf("%c", c[i]);
    }
    printf("\n");

    // Generate the signature of the hash
    const char* specificed_hash_hexstr = "ffffffffffffffffffffffffffffffffffffffffffffffff86ec2cd33d40aeaef14731a3ce4bca2a0147a0155997f573"; // Santosh
    printf("Given hash key string = %s\n", specificed_hash_hexstr);
    BIGNUM* hash_bn = BN_new();
    BN_hex2bn(&hash_bn, specificed_hash_hexstr);
    unsigned char* hash_bytes = (unsigned char*)malloc(BN_num_bytes(hash_bn));
    BN_bn2bin(hash_bn, hash_bytes);
    printf("Hash:\n\tE :  %s\n", BN_bn2hex(hash_bn));

    const ECDSA_SIG* signature = ECDSA_do_sign(hash_bytes, BN_num_bytes(hash_bn), eckey);
    const char* signature_r_hex_str = BN_bn2hex(signature->r);
    const char* signature_s_hex_str = BN_bn2hex(signature->s);

    printf("Signature:\n");
    printf("\tR :  %s\n", signature_r_hex_str);
    printf("\tS :  %s\n", signature_s_hex_str);

    printf("Done\n");

    BN_CTX_free(ctx);
    free(cc);
    free(hash_bytes);
    BN_free(hash_bn);

    return 0;
}

//      ECDSA_WRITE_AX : ecdsa_block_din = 384'haa87ca22be8b05378eb1c71ef320ad746e1d3b628ba79b9859f741e082542a385502f25dbf55296c3a545e3872760ab7; //GX
//      ECDSA_WRITE_AY : ecdsa_block_din = 384'h3617de4a96262c6f5d9e98bf9292dc29f8f41dbd289a147ce9da3113b5f0b8c00a60b1ce1d7e819d7a431d7c90ea0e5f; //GY
//      ECDSA_WRITE_BX : ecdsa_block_din = 384'hfda78b0ab4ac4b0ec6c66ac03ceb861637f0e2a2503f92be4abbf2b401e878caa799429b6cd63d7d7ab5dff1cc0a961b; //QX (CX)
//      ECDSA_WRITE_BY : ecdsa_block_din = 384'ha07404ae3aed5e2df24fc0bf7c580e6761a882f571ff6642b7a4a7608b16f2cda6c659b8c4d56f5d0749c1838efbb1a6; //QY (CY)
//      ECDSA_WRITE_P  : ecdsa_block_din = 384'hfffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffeffffffff0000000000000000ffffffff; //P
//      ECDSA_WRITE_A  : ecdsa_block_din = 384'hfffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffeffffffff0000000000000000fffffffc; //A
//      ECDSA_WRITE_N  : ecdsa_block_din = 384'hffffffffffffffffffffffffffffffffffffffffffffffffc7634d81f4372ddf581a0db248b0a77aecec196accc52973; //N
//      ECDSA_WRITE_R  : ecdsa_block_din = 384'hfda78b0ab4ac4b0ec6c66ac03ceb861637f0e2a2503f92be4abbf2b401e878caa799429b6cd63d7d7ab5dff1cc0a961b; // R of the Signature
//      ECDSA_WRITE_S  : ecdsa_block_din = 384'h167cad0a5efef80c66af3a381a725eeaef6155e2de10024574bca90d8127e15a839b9f079bd80333fb085c8aadba987e; // S of the Signature
//      ECDSA_WRITE_E  : ecdsa_block_din = 384'hffffffffffffffffffffffffffffffffffffffffffffffff86ec2cd33d40aeaef14731a3ce4bca2a0147a0155997f573; // Message hash

