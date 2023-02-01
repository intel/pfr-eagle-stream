// Standard headers
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>
#include <unordered_map>
#include <openssl/ec.h>
#include <openssl/obj_mac.h>
#include <openssl/bn.h>
#include <openssl/objects.h>
#include <openssl/ecdsa.h>
#include <openssl/evp.h>
#include <openssl/err.h>

// Test headers
#include "bsp_mock.h"
#include "crypto_mock.h"

// Code headers

// Static data

// Type definitions

CRYPTO_MOCK::CRYPTO_MOCK() :
    m_crypto_state(CRYPTO_STATE::WAIT_CRYPTO_START),
    m_ec_or_sha(EC_OR_SHA_STATE::SHA_256_AND_EC),
    m_dma_state(DMA_STATE::ACCEPT_SHA_384_DMA_DATA),
    m_ec_pub_key_state(EC_PUB_KEY_STATE::EMPTY_STATE),
    m_ec_sig_state(EC_SIG_STATE::EMPTY_STATE),
    m_entropy_state(ENTROPY_STATE::EMPTY_STATE),
    m_entropy_health_error(0),
    m_entropy_synthetic_seed(0),
    m_pubkey_gen(0),
    m_sig_gen(0),
    m_bx_check(0),
    m_by_check(0),
    m_sha_last_block_counter(0),
    m_data_length(0),
    m_data_length_dma(0),
    m_dma_go_bit(0),
    m_dma_mux(2),
    m_padding_start(0),
    m_padding_end(0),
    m_ufm_or_spi_starting_address(0),
    m_end_of_block(0),
    m_first(0),
    m_dma_done(0),
    m_cx_verified(0),
    m_padding_counter(0),
    m_cur_transfer_size(0),
    m_crypto_data_idx(0),
    m_sha_block_counter(0),
    m_crypto_calc_pass(false)
{

    // Clear all vectors
    for (auto& it : m_crypto_data_384)
    {
        it.fill(0);
    }
    for (auto& it : m_crypto_data_256)
    {
        it.fill(0);
    }
    
    // Clear the stored temporary data
    for (alt_u32 word_i = 0; word_i < (SHA384_LENGTH / 4); word_i++)
    {
        m_calculated_sha_384[word_i] = 0;
        m_pub_key_cx_384[word_i] = 0;
        m_pub_key_cy_384[word_i] = 0;
        m_sig_r_384[word_i] = 0;
        m_sig_s_384[word_i] = 0;
    }

    for (alt_u32 word_i = 0; word_i < (SHA256_LENGTH / 4); word_i++)
    {
        m_calculated_sha_256[word_i] = 0;
        m_pub_key_cx_256[word_i] = 0;
        m_pub_key_cy_256[word_i] = 0;
        m_sig_r_256[word_i] = 0;
        m_sig_s_256[word_i] = 0;
    }

    for (alt_u32 word_i = 0; word_i < 64; word_i++)
    {
    	m_entropy_sample[word_i] = 0;
    }

    // OpenSSL Initialization
    OpenSSL_add_all_algorithms();

}

CRYPTO_MOCK::~CRYPTO_MOCK()
{
    EVP_cleanup();
}

//read from mock
alt_u32 CRYPTO_MOCK::get_mem_word(void* addr)
{
    // Returns all the cryptographic calculated value.
    // For example, returning the hash value, public keys, etc...
    // Also returns important state like SHA DONE, etc etc...
    alt_u32* addr_int = reinterpret_cast<alt_u32*>(addr);

    if (addr_int == CRYPTO_GPO_1_ADDR)
    {
        return 0;
    }
    else if (addr_int == CRYPTO_DMA_CSR_ADDRESS)
    {
        if ((m_dma_state == DMA_STATE::DMA_DONE) && (m_dma_go_bit == 1))
        {
            // Here means that the dma is connected to spi
            // Returns dma csr done bit when the conditions are fulfilled
            m_dma_done |= CRYPTO_DMA_CSR_DONE_MASK;
        }
        else if ((m_dma_state == DMA_STATE::DMA_DONE) && (m_dma_go_bit == 2))
        {
            // Dma is connected to ufm
            // Returns dma csr done bit when the conditions are fulfilled
            m_dma_done |= CRYPTO_DMA_CSR_DONE_MASK;
        }
        else
        {
            m_dma_done = 0;
        }
        return m_dma_done;
    }
    else if (addr_int == CRYPTO_CSR_ADDR)
    {
        if ((m_crypto_state == CRYPTO_STATE::CRYPTO_CALC_DONE) && (m_entropy_state == ENTROPY_STATE::EMPTY_STATE))
        {
            alt_u32 ret = 0;

            if (m_padding_end == 1)
            {
                // Whenever padding is finished, compute cryptography hashing
                // Applies for both DMA and NIOS padding
                compute_crypto_calculation();
                m_sha_block_counter = 0;
                m_padding_end = 0;
            }

            if (m_ec_or_sha == EC_OR_SHA_STATE::SHA_384_ONLY) // for sha 384
            {
                if ((m_dma_state == DMA_STATE::DMA_DONE) && (m_dma_go_bit == 1))
                {
                    // If crypto mock is done after DMA is executed plus conditions fulfilled
                    ret |= CRYPTO_CSR_SHA_DONE_CURRENT_BLOCK_MSK;
                }
                else if ((m_dma_state == DMA_STATE::DMA_DONE) && (m_dma_go_bit == 2))
                {
                    // If crypto mock is done after DMA is executed plus conditions fulfilled
                    ret |= CRYPTO_CSR_SHA_DONE_CURRENT_BLOCK_MSK;
                }
                else
                {
                    // If crypto mock is done after NIOS solution (padding from Nios)
                    m_crypto_state = CRYPTO_STATE::ACCEPT_SHA_384_DATA;
                    ret |= CRYPTO_CSR_SHA_DONE_CURRENT_BLOCK_MSK;
                }
            }
            else if (m_ec_or_sha == EC_OR_SHA_STATE::SHA_256_ONLY) // for sha 256
            {
                if ((m_dma_state == DMA_STATE::DMA_DONE) && (m_dma_go_bit == 1))
                {
                    ret |= CRYPTO_CSR_SHA_DONE_CURRENT_BLOCK_MSK;
                }
                else if ((m_dma_state == DMA_STATE::DMA_DONE) && (m_dma_go_bit == 2))
                {
                    // If crypto mock is done after DMA is executed plus conditions fulfilled
                    ret |= CRYPTO_CSR_SHA_DONE_CURRENT_BLOCK_MSK;
                }
                else
                {
                    m_crypto_state = CRYPTO_STATE::ACCEPT_SHA_256_DATA;
                    ret |= CRYPTO_CSR_SHA_DONE_CURRENT_BLOCK_MSK;
                }
            }
            else if (m_ec_or_sha == EC_OR_SHA_STATE::SHA_256_AND_EC || m_ec_or_sha == EC_OR_SHA_STATE::SHA_384_AND_EC)
            {
                // This is for ecdsa operation.
                // Returns ecdsa valid mask
                ret |= CRYPTO_CSR_ECDSA_DOUT_VALID_MSK;
                if (m_crypto_calc_pass)
                {
                    // ecdsa calculation passed.
                    // Returns 0
                    m_cx_verified = 0;
                }
                else
                {
                    m_cx_verified = 1;
                }
            }
            return ret;
        }
        else if (m_entropy_state == ENTROPY_STATE::ENTROPY_AVAILABLE)
        {
            alt_u32 full = 0;
            entropy_source_computation();
            full |= CRYPTO_CSR_ENTROPY_SOURCE_FIFO_FULL_MSK;
            if (m_entropy_health_error == 0xFFFFFFFF)
            {
                full |= CRYPTO_CSR_ENTROPY_SOURCE_HEALTH_TEST_ERROR_MSK;
            }
            return full;
        }
    }
    else if (addr_int == CRYPTO_ENTROPY_SOURCE_OUTPUT_ADDR(0))
    {
        if (m_entropy_synthetic_seed == 11)
        {
        	m_entropy_synthetic_seed = 0;
        }
        return m_entropy_sample[m_entropy_synthetic_seed++];
    }
    else if (addr_int == CRYPTO_ECDSA_DATA_CX_OUTPUT_ADDR(0))
    {
        if ((m_ec_pub_key_state == EC_PUB_KEY_STATE::EMPTY_STATE) && (m_ec_sig_state == EC_SIG_STATE::EMPTY_STATE))
        {
            if ((m_ec_or_sha == EC_OR_SHA_STATE::SHA_384_AND_EC) || (m_ec_or_sha == EC_OR_SHA_STATE::SHA_256_AND_EC))
            {
                return m_cx_verified;
            }
        }
        else if (m_ec_pub_key_state == EC_PUB_KEY_STATE::GEN_PUB_KEY_384)
        {
            return m_pub_key_cx_384[11];
        }
        else if (m_ec_pub_key_state == EC_PUB_KEY_STATE::GEN_PUB_KEY_256)
        {
            return m_pub_key_cx_256[7];
        }
        else if (m_ec_sig_state == EC_SIG_STATE::GEN_SIG_384)
        {
            return m_sig_r_384[11];
        }
        else if (m_ec_sig_state == EC_SIG_STATE::GEN_SIG_256)
        {
            return m_sig_r_256[7];
        }
    }
    else if (addr_int == CRYPTO_ECDSA_DATA_CX_OUTPUT_ADDR(1))
    {
        if ((m_ec_pub_key_state == EC_PUB_KEY_STATE::EMPTY_STATE) && (m_ec_sig_state == EC_SIG_STATE::EMPTY_STATE))
        {
            if ((m_ec_or_sha == EC_OR_SHA_STATE::SHA_384_AND_EC) || (m_ec_or_sha == EC_OR_SHA_STATE::SHA_256_AND_EC))
            {
                return m_cx_verified;
            }
        }
        else if (m_ec_pub_key_state == EC_PUB_KEY_STATE::GEN_PUB_KEY_384)
        {
            return m_pub_key_cx_384[10];
        }
        else if (m_ec_pub_key_state == EC_PUB_KEY_STATE::GEN_PUB_KEY_256)
        {
            return m_pub_key_cx_256[6];
        }
        else if (m_ec_sig_state == EC_SIG_STATE::GEN_SIG_384)
        {
            return m_sig_r_384[10];
        }
        else if (m_ec_sig_state == EC_SIG_STATE::GEN_SIG_256)
        {
            return m_sig_r_256[6];
        }
    }
    else if (addr_int == CRYPTO_ECDSA_DATA_CX_OUTPUT_ADDR(2))
    {
        if ((m_ec_pub_key_state == EC_PUB_KEY_STATE::EMPTY_STATE) && (m_ec_sig_state == EC_SIG_STATE::EMPTY_STATE))
        {
            if ((m_ec_or_sha == EC_OR_SHA_STATE::SHA_384_AND_EC) || (m_ec_or_sha == EC_OR_SHA_STATE::SHA_256_AND_EC))
            {
                return m_cx_verified;
            }
        }
        else if (m_ec_pub_key_state == EC_PUB_KEY_STATE::GEN_PUB_KEY_384)
        {
            return m_pub_key_cx_384[9];
        }
        else if (m_ec_pub_key_state == EC_PUB_KEY_STATE::GEN_PUB_KEY_256)
        {
            return m_pub_key_cx_256[5];
        }
        else if (m_ec_sig_state == EC_SIG_STATE::GEN_SIG_384)
        {
            return m_sig_r_384[9];
        }
        else if (m_ec_sig_state == EC_SIG_STATE::GEN_SIG_256)
        {
            return m_sig_r_256[5];
        }
    }
    else if (addr_int == CRYPTO_ECDSA_DATA_CX_OUTPUT_ADDR(3))
    {
        if ((m_ec_pub_key_state == EC_PUB_KEY_STATE::EMPTY_STATE) && (m_ec_sig_state == EC_SIG_STATE::EMPTY_STATE))
        {
            if ((m_ec_or_sha == EC_OR_SHA_STATE::SHA_384_AND_EC) || (m_ec_or_sha == EC_OR_SHA_STATE::SHA_256_AND_EC))
            {
                return m_cx_verified;
            }
        }
        else if (m_ec_pub_key_state == EC_PUB_KEY_STATE::GEN_PUB_KEY_384)
        {
            return m_pub_key_cx_384[8];
        }
        else if (m_ec_pub_key_state == EC_PUB_KEY_STATE::GEN_PUB_KEY_256)
        {
            return m_pub_key_cx_256[4];
        }
        else if (m_ec_sig_state == EC_SIG_STATE::GEN_SIG_384)
        {
            return m_sig_r_384[8];
        }
        else if (m_ec_sig_state == EC_SIG_STATE::GEN_SIG_256)
        {
            return m_sig_r_256[4];
        }
    }
    else if (addr_int == CRYPTO_ECDSA_DATA_CX_OUTPUT_ADDR(4))
    {
        if ((m_ec_pub_key_state == EC_PUB_KEY_STATE::EMPTY_STATE) && (m_ec_sig_state == EC_SIG_STATE::EMPTY_STATE))
        {
            if ((m_ec_or_sha == EC_OR_SHA_STATE::SHA_384_AND_EC) || (m_ec_or_sha == EC_OR_SHA_STATE::SHA_256_AND_EC))
            {
                return m_cx_verified;
            }
        }
        else if (m_ec_pub_key_state == EC_PUB_KEY_STATE::GEN_PUB_KEY_384)
        {
            return m_pub_key_cx_384[7];
        }
        else if (m_ec_pub_key_state == EC_PUB_KEY_STATE::GEN_PUB_KEY_256)
        {
            return m_pub_key_cx_256[3];
        }
        else if (m_ec_sig_state == EC_SIG_STATE::GEN_SIG_384)
        {
            return m_sig_r_384[7];
        }
        else if (m_ec_sig_state == EC_SIG_STATE::GEN_SIG_256)
        {
            return m_sig_r_256[3];
        }
    }
    else if (addr_int == CRYPTO_ECDSA_DATA_CX_OUTPUT_ADDR(5))
    {
        if ((m_ec_pub_key_state == EC_PUB_KEY_STATE::EMPTY_STATE) && (m_ec_sig_state == EC_SIG_STATE::EMPTY_STATE))
        {
            if ((m_ec_or_sha == EC_OR_SHA_STATE::SHA_384_AND_EC) || (m_ec_or_sha == EC_OR_SHA_STATE::SHA_256_AND_EC))
            {
                return m_cx_verified;
            }
        }
        else if (m_ec_pub_key_state == EC_PUB_KEY_STATE::GEN_PUB_KEY_384)
        {
            return m_pub_key_cx_384[6];
        }
        else if (m_ec_pub_key_state == EC_PUB_KEY_STATE::GEN_PUB_KEY_256)
        {
            return m_pub_key_cx_256[2];
        }
        else if (m_ec_sig_state == EC_SIG_STATE::GEN_SIG_384)
        {
            return m_sig_r_384[6];
        }
        else if (m_ec_sig_state == EC_SIG_STATE::GEN_SIG_256)
        {
            return m_sig_r_256[2];
        }
    }
    else if (addr_int == CRYPTO_ECDSA_DATA_CX_OUTPUT_ADDR(6))
    {
        if ((m_ec_pub_key_state == EC_PUB_KEY_STATE::EMPTY_STATE) && (m_ec_sig_state == EC_SIG_STATE::EMPTY_STATE))
        {
            if ((m_ec_or_sha == EC_OR_SHA_STATE::SHA_384_AND_EC) || (m_ec_or_sha == EC_OR_SHA_STATE::SHA_256_AND_EC))
            {
                return m_cx_verified;
            }
        }
        else if (m_ec_pub_key_state == EC_PUB_KEY_STATE::GEN_PUB_KEY_384)
        {
            return m_pub_key_cx_384[5];
        }
        else if (m_ec_pub_key_state == EC_PUB_KEY_STATE::GEN_PUB_KEY_256)
        {
            return m_pub_key_cx_256[1];
        }
        else if (m_ec_sig_state == EC_SIG_STATE::GEN_SIG_384)
        {
            return m_sig_r_384[5];
        }
        else if (m_ec_sig_state == EC_SIG_STATE::GEN_SIG_256)
        {
            return m_sig_r_256[1];
        }
    }
    else if (addr_int == CRYPTO_ECDSA_DATA_CX_OUTPUT_ADDR(7))
    {
        if ((m_ec_pub_key_state == EC_PUB_KEY_STATE::EMPTY_STATE) && (m_ec_sig_state == EC_SIG_STATE::EMPTY_STATE))
        {
            if ((m_ec_or_sha == EC_OR_SHA_STATE::SHA_384_AND_EC) || (m_ec_or_sha == EC_OR_SHA_STATE::SHA_256_AND_EC))
            {
                return m_cx_verified;
            }
        }
        else if (m_ec_pub_key_state == EC_PUB_KEY_STATE::GEN_PUB_KEY_384)
        {
            return m_pub_key_cx_384[4];
        }
        else if (m_ec_pub_key_state == EC_PUB_KEY_STATE::GEN_PUB_KEY_256)
        {
            return m_pub_key_cx_256[0];
        }
        else if (m_ec_sig_state == EC_SIG_STATE::GEN_SIG_384)
        {
            return m_sig_r_384[4];
        }
        else if (m_ec_sig_state == EC_SIG_STATE::GEN_SIG_256)
        {
            return m_sig_r_256[0];
        }
    }
    else if (addr_int == CRYPTO_ECDSA_DATA_CX_OUTPUT_ADDR(8))
    {
        if ((m_ec_pub_key_state == EC_PUB_KEY_STATE::EMPTY_STATE) && (m_ec_sig_state == EC_SIG_STATE::EMPTY_STATE))
        {
            if ((m_ec_or_sha == EC_OR_SHA_STATE::SHA_384_AND_EC) || (m_ec_or_sha == EC_OR_SHA_STATE::SHA_256_AND_EC))
            {
                return m_cx_verified;
            }
        }
        else if (m_ec_pub_key_state == EC_PUB_KEY_STATE::GEN_PUB_KEY_384)
        {
            return m_pub_key_cx_384[3];
        }
        else if (m_ec_sig_state == EC_SIG_STATE::GEN_SIG_384)
        {
            return m_sig_r_384[3];
        }
    }
    else if (addr_int == CRYPTO_ECDSA_DATA_CX_OUTPUT_ADDR(9))
    {
        if ((m_ec_pub_key_state == EC_PUB_KEY_STATE::EMPTY_STATE) && (m_ec_sig_state == EC_SIG_STATE::EMPTY_STATE))
        {
            if ((m_ec_or_sha == EC_OR_SHA_STATE::SHA_384_AND_EC) || (m_ec_or_sha == EC_OR_SHA_STATE::SHA_256_AND_EC))
            {
                return m_cx_verified;
            }
        }
        else if (m_ec_pub_key_state == EC_PUB_KEY_STATE::GEN_PUB_KEY_384)
        {
            return m_pub_key_cx_384[2];
        }
        else if (m_ec_sig_state == EC_SIG_STATE::GEN_SIG_384)
        {
            return m_sig_r_384[2];
        }
    }
    else if (addr_int == CRYPTO_ECDSA_DATA_CX_OUTPUT_ADDR(10))
    {
        if ((m_ec_pub_key_state == EC_PUB_KEY_STATE::EMPTY_STATE) && (m_ec_sig_state == EC_SIG_STATE::EMPTY_STATE))
        {
            if ((m_ec_or_sha == EC_OR_SHA_STATE::SHA_384_AND_EC) || (m_ec_or_sha == EC_OR_SHA_STATE::SHA_256_AND_EC))
            {
                return m_cx_verified;
            }
        }
        else if (m_ec_pub_key_state == EC_PUB_KEY_STATE::GEN_PUB_KEY_384)
        {
            return m_pub_key_cx_384[1];
        }
        else if (m_ec_sig_state == EC_SIG_STATE::GEN_SIG_384)
        {
            return m_sig_r_384[1];
        }
    }
    else if (addr_int == CRYPTO_ECDSA_DATA_CX_OUTPUT_ADDR(11))
    {
        if ((m_ec_pub_key_state == EC_PUB_KEY_STATE::EMPTY_STATE) && (m_ec_sig_state == EC_SIG_STATE::EMPTY_STATE))
        {
            if ((m_ec_or_sha == EC_OR_SHA_STATE::SHA_384_AND_EC) || (m_ec_or_sha == EC_OR_SHA_STATE::SHA_256_AND_EC))
            {
                return m_cx_verified;
            }
        }
        else if (m_ec_pub_key_state == EC_PUB_KEY_STATE::GEN_PUB_KEY_384)
        {
            return m_pub_key_cx_384[0];
        }
        else if (m_ec_sig_state == EC_SIG_STATE::GEN_SIG_384)
        {
            return m_sig_r_384[0];
        }
    }
    else if (addr_int == CRYPTO_ECDSA_DATA_CY_OUTPUT_ADDR(0))
    {
        if (m_ec_pub_key_state == EC_PUB_KEY_STATE::GEN_PUB_KEY_384)
        {
            return m_pub_key_cy_384[11];
        }
        else if (m_ec_pub_key_state == EC_PUB_KEY_STATE::GEN_PUB_KEY_256)
        {
            return m_pub_key_cy_256[7];
        }
        else if (m_ec_sig_state == EC_SIG_STATE::GEN_SIG_384)
        {
            return m_sig_s_384[11];
        }
        else if (m_ec_sig_state == EC_SIG_STATE::GEN_SIG_256)
        {
            return m_sig_s_256[7];
        }
    }
    else if (addr_int == CRYPTO_ECDSA_DATA_CY_OUTPUT_ADDR(1))
    {
        if (m_ec_pub_key_state == EC_PUB_KEY_STATE::GEN_PUB_KEY_384)
        {
            return m_pub_key_cy_384[10];
        }
        else if (m_ec_pub_key_state == EC_PUB_KEY_STATE::GEN_PUB_KEY_256)
        {
            return m_pub_key_cy_256[6];
        }
        else if (m_ec_sig_state == EC_SIG_STATE::GEN_SIG_384)
        {
            return m_sig_s_384[10];
        }
        else if (m_ec_sig_state == EC_SIG_STATE::GEN_SIG_256)
        {
            return m_sig_s_256[6];
        }
    }
    else if (addr_int == CRYPTO_ECDSA_DATA_CY_OUTPUT_ADDR(2))
    {
        if (m_ec_pub_key_state == EC_PUB_KEY_STATE::GEN_PUB_KEY_384)
        {
            return m_pub_key_cy_384[9];
        }
        else if (m_ec_pub_key_state == EC_PUB_KEY_STATE::GEN_PUB_KEY_256)
        {
            return m_pub_key_cy_256[5];
        }
        else if (m_ec_sig_state == EC_SIG_STATE::GEN_SIG_384)
        {
            return m_sig_s_384[9];
        }
        else if (m_ec_sig_state == EC_SIG_STATE::GEN_SIG_256)
        {
            return m_sig_s_256[5];
        }
    }
    else if (addr_int == CRYPTO_ECDSA_DATA_CY_OUTPUT_ADDR(3))
    {
        if (m_ec_pub_key_state == EC_PUB_KEY_STATE::GEN_PUB_KEY_384)
        {
            return m_pub_key_cy_384[8];
        }
        else if (m_ec_pub_key_state == EC_PUB_KEY_STATE::GEN_PUB_KEY_256)
        {
            return m_pub_key_cy_256[4];
        }
        else if (m_ec_sig_state == EC_SIG_STATE::GEN_SIG_384)
        {
            return m_sig_s_384[8];
        }
        else if (m_ec_sig_state == EC_SIG_STATE::GEN_SIG_256)
        {
            return m_sig_s_256[4];
        }
    }
    else if (addr_int == CRYPTO_ECDSA_DATA_CY_OUTPUT_ADDR(4))
    {
        if (m_ec_pub_key_state == EC_PUB_KEY_STATE::GEN_PUB_KEY_384)
        {
            return m_pub_key_cy_384[7];
        }
        else if (m_ec_pub_key_state == EC_PUB_KEY_STATE::GEN_PUB_KEY_256)
        {
            return m_pub_key_cy_256[3];
        }
        else if (m_ec_sig_state == EC_SIG_STATE::GEN_SIG_384)
        {
            return m_sig_s_384[7];
        }
        else if (m_ec_sig_state == EC_SIG_STATE::GEN_SIG_256)
        {
            return m_sig_s_256[3];
        }
    }
    else if (addr_int == CRYPTO_ECDSA_DATA_CY_OUTPUT_ADDR(5))
    {
        if (m_ec_pub_key_state == EC_PUB_KEY_STATE::GEN_PUB_KEY_384)
        {
            return m_pub_key_cy_384[6];
        }
        else if (m_ec_pub_key_state == EC_PUB_KEY_STATE::GEN_PUB_KEY_256)
        {
            return m_pub_key_cy_256[2];
        }
        else if (m_ec_sig_state == EC_SIG_STATE::GEN_SIG_384)
        {
            return m_sig_s_384[6];
        }
        else if (m_ec_sig_state == EC_SIG_STATE::GEN_SIG_256)
        {
            return m_sig_s_256[2];
        }
    }
    else if (addr_int == CRYPTO_ECDSA_DATA_CY_OUTPUT_ADDR(6))
    {
        if (m_ec_pub_key_state == EC_PUB_KEY_STATE::GEN_PUB_KEY_384)
        {
            return m_pub_key_cy_384[5];
        }
        else if (m_ec_pub_key_state == EC_PUB_KEY_STATE::GEN_PUB_KEY_256)
        {
            return m_pub_key_cy_256[1];
        }
        else if (m_ec_sig_state == EC_SIG_STATE::GEN_SIG_384)
        {
            return m_sig_s_384[5];
        }
        else if (m_ec_sig_state == EC_SIG_STATE::GEN_SIG_256)
        {
            return m_sig_s_256[1];
        }
    }
    else if (addr_int == CRYPTO_ECDSA_DATA_CY_OUTPUT_ADDR(7))
    {
        if (m_ec_pub_key_state == EC_PUB_KEY_STATE::GEN_PUB_KEY_384)
        {
            return m_pub_key_cy_384[4];
        }
        else if (m_ec_pub_key_state == EC_PUB_KEY_STATE::GEN_PUB_KEY_256)
        {
            return m_pub_key_cy_256[0];
        }
        else if (m_ec_sig_state == EC_SIG_STATE::GEN_SIG_384)
        {
            return m_sig_s_384[4];
        }
        else if (m_ec_sig_state == EC_SIG_STATE::GEN_SIG_256)
        {
            return m_sig_s_256[0];
        }
    }
    else if (addr_int == CRYPTO_ECDSA_DATA_CY_OUTPUT_ADDR(8))
    {
        if (m_ec_pub_key_state == EC_PUB_KEY_STATE::GEN_PUB_KEY_384)
        {
            return m_pub_key_cy_384[3];
        }
        else if (m_ec_sig_state == EC_SIG_STATE::GEN_SIG_384)
        {
            return m_sig_s_384[3];
        }
    }
    else if (addr_int == CRYPTO_ECDSA_DATA_CY_OUTPUT_ADDR(9))
    {
        if (m_ec_pub_key_state == EC_PUB_KEY_STATE::GEN_PUB_KEY_384)
        {
            return m_pub_key_cy_384[2];
        }
        else if (m_ec_sig_state == EC_SIG_STATE::GEN_SIG_384)
        {
            return m_sig_s_384[2];
        }
    }
    else if (addr_int == CRYPTO_ECDSA_DATA_CY_OUTPUT_ADDR(10))
    {
        if (m_ec_pub_key_state == EC_PUB_KEY_STATE::GEN_PUB_KEY_384)
        {
            return m_pub_key_cy_384[1];
        }
        else if (m_ec_sig_state == EC_SIG_STATE::GEN_SIG_384)
        {
            return m_sig_s_384[1];
        }
    }
    else if (addr_int == CRYPTO_ECDSA_DATA_CY_OUTPUT_ADDR(11))
    {
        if (m_ec_pub_key_state == EC_PUB_KEY_STATE::GEN_PUB_KEY_384)
        {
            return m_pub_key_cy_384[0];
        }
        else if (m_ec_sig_state == EC_SIG_STATE::GEN_SIG_384)
        {
            return m_sig_s_384[0];
        }
    }
    else if (addr_int == CRYPTO_DATA_SHA_ADDR(0))
    {
       	if ((m_ec_or_sha == EC_OR_SHA_STATE::SHA_256_ONLY) || (m_ec_or_sha == EC_OR_SHA_STATE::SHA_256_AND_EC))
       	{
       	    return m_calculated_sha_256[7];
       	}
       	else if (m_ec_or_sha == EC_OR_SHA_STATE::SHA_384_ONLY || (m_ec_or_sha == EC_OR_SHA_STATE::SHA_384_AND_EC))
       	{
       	    return m_calculated_sha_384[11];
       	}
    }
    else if (addr_int == CRYPTO_DATA_SHA_ADDR(1))
    {
    	if ((m_ec_or_sha == EC_OR_SHA_STATE::SHA_256_ONLY) || (m_ec_or_sha == EC_OR_SHA_STATE::SHA_256_AND_EC))
    	{
    		return m_calculated_sha_256[6];
    	}
    	else if (m_ec_or_sha == EC_OR_SHA_STATE::SHA_384_ONLY || (m_ec_or_sha == EC_OR_SHA_STATE::SHA_384_AND_EC))
    	{
            return m_calculated_sha_384[10];
    	}
    }
    else if (addr_int == CRYPTO_DATA_SHA_ADDR(2))
    {
    	if ((m_ec_or_sha == EC_OR_SHA_STATE::SHA_256_ONLY) || (m_ec_or_sha == EC_OR_SHA_STATE::SHA_256_AND_EC))
    	{
    		return m_calculated_sha_256[5];
    	}
    	else if (m_ec_or_sha == EC_OR_SHA_STATE::SHA_384_ONLY || (m_ec_or_sha == EC_OR_SHA_STATE::SHA_384_AND_EC))
    	{
    		return m_calculated_sha_384[9];
    	}
    }
    else if (addr_int == CRYPTO_DATA_SHA_ADDR(3))
    {
    	if ((m_ec_or_sha == EC_OR_SHA_STATE::SHA_256_ONLY) || (m_ec_or_sha == EC_OR_SHA_STATE::SHA_256_AND_EC))
    	{
    		return m_calculated_sha_256[4];
    	}
    	else if (m_ec_or_sha == EC_OR_SHA_STATE::SHA_384_ONLY || (m_ec_or_sha == EC_OR_SHA_STATE::SHA_384_AND_EC))
    	{
    		return m_calculated_sha_384[8];
    	}
    }
    else if (addr_int == CRYPTO_DATA_SHA_ADDR(4))
    {
    	if ((m_ec_or_sha == EC_OR_SHA_STATE::SHA_256_ONLY) || (m_ec_or_sha == EC_OR_SHA_STATE::SHA_256_AND_EC))
    	{
    		return m_calculated_sha_256[3];
    	}
    	else if (m_ec_or_sha == EC_OR_SHA_STATE::SHA_384_ONLY || (m_ec_or_sha == EC_OR_SHA_STATE::SHA_384_AND_EC))
    	{
    		return m_calculated_sha_384[7];
    	}
    }
    else if (addr_int == CRYPTO_DATA_SHA_ADDR(5))
    {
    	if ((m_ec_or_sha == EC_OR_SHA_STATE::SHA_256_ONLY) || (m_ec_or_sha == EC_OR_SHA_STATE::SHA_256_AND_EC))
    	{
    		return m_calculated_sha_256[2];
    	}
    	else if (m_ec_or_sha == EC_OR_SHA_STATE::SHA_384_ONLY || (m_ec_or_sha == EC_OR_SHA_STATE::SHA_384_AND_EC))
    	{
    		return m_calculated_sha_384[6];
    	}
    }
    else if (addr_int == CRYPTO_DATA_SHA_ADDR(6))
    {
    	if ((m_ec_or_sha == EC_OR_SHA_STATE::SHA_256_ONLY) || (m_ec_or_sha == EC_OR_SHA_STATE::SHA_256_AND_EC))
    	{
    		return m_calculated_sha_256[1];
    	}
    	else if (m_ec_or_sha == EC_OR_SHA_STATE::SHA_384_ONLY || (m_ec_or_sha == EC_OR_SHA_STATE::SHA_384_AND_EC))
    	{
    		return m_calculated_sha_384[5];
    	}
    }
    else if (addr_int == CRYPTO_DATA_SHA_ADDR(7))
    {
    	if ((m_ec_or_sha == EC_OR_SHA_STATE::SHA_256_ONLY) || (m_ec_or_sha == EC_OR_SHA_STATE::SHA_256_AND_EC))
    	{
    		return m_calculated_sha_256[0];
    	}
    	else if (m_ec_or_sha == EC_OR_SHA_STATE::SHA_384_ONLY || (m_ec_or_sha == EC_OR_SHA_STATE::SHA_384_AND_EC))
    	{
    		return m_calculated_sha_384[4];
    	}
    }
    else if (addr_int == CRYPTO_DATA_SHA_ADDR(8))
    {
        return m_calculated_sha_384[3];
    }
    else if (addr_int == CRYPTO_DATA_SHA_ADDR(9))
    {
        return m_calculated_sha_384[2];
    }
    else if (addr_int == CRYPTO_DATA_SHA_ADDR(10))
    {
        return m_calculated_sha_384[1];
    }
    else if (addr_int == CRYPTO_DATA_SHA_ADDR(11))
    {
        return m_calculated_sha_384[0];
    }
    else
    {
        PFR_INTERNAL_ERROR("Undefined handler for address");
    }
    return 0;
}

//write to mock
void CRYPTO_MOCK::set_mem_word(void* addr, alt_u32 data)
{
    alt_u32* addr_int = reinterpret_cast<alt_u32*>(addr);

    if (addr_int == CRYPTO_CSR_ADDR)
    {
        // The here tracks the state sent by fw and do appropriate operations.
        m_crypto_calc_pass = false;

        // When the crypto block is being reset, clear all the data stored in the arrays
        if (((data >> 31) & 0x00000001) == 1)
        {
            m_data_length = 0;
            m_cur_transfer_size = 0;

            // Resize the sha data to reallocate
            m_sha_data.resize(0);

            // Clear all vectors
            for (auto it : m_crypto_data_256)
            {
                it.fill(0);
            }

            for (auto it : m_crypto_data_384)
            {
                it.fill(0);
            }

            // Clear the stored temporary data
            for (alt_u32 word_i = 0; word_i < (SHA384_LENGTH / 4); word_i++)
            {
                m_calculated_sha_384[word_i] = 0;
                m_pub_key_cx_384[word_i] = 0;
                m_pub_key_cy_384[word_i] = 0;
                m_sig_r_384[word_i] = 0;
                m_sig_s_384[word_i] = 0;
            }

            for (alt_u32 word_i = 0; word_i < (SHA256_LENGTH / 4); word_i++)
            {
                m_calculated_sha_256[word_i] = 0;
                m_pub_key_cx_256[word_i] = 0;
                m_pub_key_cy_256[word_i] = 0;
                m_sig_r_256[word_i] = 0;
                m_sig_s_256[word_i] = 0;
            }

            for (alt_u32 word_i = 0; word_i < 64; word_i++)
            {
                m_entropy_sample[word_i] = 0;
            }

            m_entropy_synthetic_seed = 0;
            m_pubkey_gen = 0;
            m_sig_gen = 0;
            m_bx_check = 0;
            m_by_check = 0;
            m_sha_last_block_counter = 0;
            m_data_length = 0;
            m_data_length_dma = 0;
            m_dma_go_bit = 0;
            m_dma_mux = 2;
            m_padding_start = 0;
            m_padding_end = 0;
            m_ufm_or_spi_starting_address = 0;
            m_end_of_block = 0;
            m_first = 0;
            m_dma_done = 0;
            m_cx_verified = 0;
            m_padding_counter = 0;
            m_cur_transfer_size = 0;
            m_crypto_data_idx = 0;
            m_sha_block_counter = 0;

            m_entropy_state = ENTROPY_STATE::EMPTY_STATE;
        }
        else if (((data >> 31) & 0x00000001) == 0)
        {
            m_sha_last_block_counter++;
        }
        if ((data == (ENTROPY_CSR_RESET_MSK | CRYPTO_256_MODE)) || (data == (ENTROPY_CSR_RESET_MSK | CRYPTO_384_MODE)))
        {
            // Reset unless forced error
            if (m_entropy_health_error != 0xFFFFFFFF)
            {
                m_entropy_health_error = 0;
            }
            m_entropy_state = ENTROPY_STATE::ENTROPY_AVAILABLE;
        }
        if (data == CRYPTO_CSR_ENTROPY_SOURCE_HEALTH_TEST_ERROR_MSK)
        {
            m_entropy_health_error = 0xFFFFFFFF;
        }
        if (data == (((CRYPTO_CSR_SHA_START_MSK | CRYPTO_CSR_SHA_LAST_MSK ) & 0xfffffffa) | CRYPTO_256_MODE))
        {
        	// SHA only
            m_crypto_state = CRYPTO_STATE::ACCEPT_SHA_256_DATA;
            m_ec_or_sha = EC_OR_SHA_STATE::SHA_256_ONLY;
            m_cur_transfer_size = 64;
        }
        else if (data == (((CRYPTO_CSR_SHA_START_MSK | CRYPTO_CSR_SHA_LAST_MSK) & 0xfffffffa) | CRYPTO_384_MODE))
        {
            m_crypto_state = CRYPTO_STATE::ACCEPT_SHA_384_DATA;
            m_ec_or_sha = EC_OR_SHA_STATE::SHA_384_ONLY;
            m_cur_transfer_size = 128;
        }
        else if ((data == ((CRYPTO_CSR_SHA_START_MSK | CRYPTO_CSR_SHA_LAST_MSK) | CRYPTO_256_MODE)) || (data == ((CRYPTO_CSR_SHA_START_MSK | CRYPTO_CSR_SHA_LAST_MSK) | CRYPTO_384_MODE)))
        {
            m_sha_last_block_counter++;
        }
        else if (data == (CRYPTO_CSR_SHA_LAST_MSK | CRYPTO_256_MODE))
        {
            m_sha_last_block_counter++;
        }
        else if (data == (CRYPTO_CSR_SHA_LAST_MSK | CRYPTO_384_MODE))
        {
            m_sha_last_block_counter++;
        }
        else if (data == (CRYPTO_CSR_ECDSA_START_MSK | CRYPTO_256_MODE))
        {
            m_cur_transfer_size = 31;
            m_entropy_state = ENTROPY_STATE::EMPTY_STATE;
        }
        else if (data == (CRYPTO_CSR_ECDSA_START_MSK | CRYPTO_384_MODE))
        {
            m_cur_transfer_size = 47;
            m_entropy_state = ENTROPY_STATE::EMPTY_STATE;
        }
        else if (data == (CRYPTO_CSR_ECDSA_INSTRUCTION_VALID_MSK | ECDSA_INSTR_WRITE_AX | CRYPTO_384_MODE))
        {
            m_crypto_state = CRYPTO_STATE::ACCEPT_EC_DATA;
            m_ec_or_sha = EC_OR_SHA_STATE::SHA_384_AND_EC;
            m_ec_pub_key_state = EC_PUB_KEY_STATE::EMPTY_STATE;

            m_cur_transfer_size = 47;
        }
        else if (data == (CRYPTO_CSR_ECDSA_INSTRUCTION_VALID_MSK | ECDSA_INSTR_WRITE_AX | CRYPTO_256_MODE))
        {
            m_crypto_state = CRYPTO_STATE::ACCEPT_EC_DATA;
            m_ec_or_sha = EC_OR_SHA_STATE::SHA_256_AND_EC;
            m_ec_pub_key_state = EC_PUB_KEY_STATE::EMPTY_STATE;

            m_cur_transfer_size = 31;
        }
        else if (data == (CRYPTO_CSR_ECDSA_INSTRUCTION_VALID_MSK | ECDSA_INSTR_WRITE_AY | CRYPTO_384_MODE))
        {
            m_cur_transfer_size = 47;
        }
        else if (data == (CRYPTO_CSR_ECDSA_INSTRUCTION_VALID_MSK | ECDSA_INSTR_WRITE_AY | CRYPTO_256_MODE))
        {
            m_cur_transfer_size = 31;
        }
        else if (data == (CRYPTO_CSR_ECDSA_INSTRUCTION_VALID_MSK | ECDSA_INSTR_WRITE_BX | CRYPTO_384_MODE))
        {
        	m_bx_check = 1;
            m_cur_transfer_size = 47;
        }
        else if (data == (CRYPTO_CSR_ECDSA_INSTRUCTION_VALID_MSK | ECDSA_INSTR_WRITE_BX | CRYPTO_256_MODE))
        {
        	m_bx_check = 1;
            m_cur_transfer_size = 31;
        }
        else if (data == (CRYPTO_CSR_ECDSA_INSTRUCTION_VALID_MSK | ECDSA_INSTR_WRITE_BY | CRYPTO_384_MODE))
        {
        	m_by_check = 1;
            m_cur_transfer_size = 47;
        }
        else if (data == (CRYPTO_CSR_ECDSA_INSTRUCTION_VALID_MSK | ECDSA_INSTR_WRITE_BY | CRYPTO_256_MODE))
        {
        	m_by_check = 1;
            m_cur_transfer_size = 31;
        }
        else if (data == (CRYPTO_CSR_ECDSA_INSTRUCTION_VALID_MSK | ECDSA_INSTR_WRITE_P | CRYPTO_384_MODE))
        {
            m_cur_transfer_size = 47;
        }
        else if (data == (CRYPTO_CSR_ECDSA_INSTRUCTION_VALID_MSK | ECDSA_INSTR_WRITE_P | CRYPTO_256_MODE))
        {
            m_cur_transfer_size = 31;
        }
        else if (data == (CRYPTO_CSR_ECDSA_INSTRUCTION_VALID_MSK | ECDSA_INSTR_WRITE_A | CRYPTO_384_MODE))
        {
            m_cur_transfer_size = 47;
        }
        else if (data == (CRYPTO_CSR_ECDSA_INSTRUCTION_VALID_MSK | ECDSA_INSTR_WRITE_A | CRYPTO_256_MODE))
        {
            m_cur_transfer_size = 31;
        }
        else if (data == (CRYPTO_CSR_ECDSA_INSTRUCTION_VALID_MSK | ECDSA_INSTR_WRITE_N | CRYPTO_384_MODE))
        {
            m_cur_transfer_size = 47;
        }
        else if (data == (CRYPTO_CSR_ECDSA_INSTRUCTION_VALID_MSK | ECDSA_INSTR_WRITE_N | CRYPTO_256_MODE))
        {
            m_cur_transfer_size = 31;
        }
        else if (data == (CRYPTO_CSR_ECDSA_INSTRUCTION_VALID_MSK | ECDSA_INSTR_WRITE_R | CRYPTO_384_MODE))
        {
            m_cur_transfer_size = 47;

            if ((m_by_check == 0) && (m_bx_check == 0))
            {
                m_pubkey_gen = 1;
                m_ec_pub_key_state = EC_PUB_KEY_STATE::GEN_PUB_KEY_384;
            }
            else if ((m_bx_check == 1) && (m_by_check == 0))
            {
            	m_sha_last_block_counter++;
            }
            else if ((m_bx_check == 1) && (m_by_check == 1))
            {
                m_ec_pub_key_state = EC_PUB_KEY_STATE::EMPTY_STATE;
            }
        }
        else if (data == (CRYPTO_CSR_ECDSA_INSTRUCTION_VALID_MSK | ECDSA_INSTR_WRITE_R | CRYPTO_256_MODE))
        {
            m_cur_transfer_size = 31;

            if ((m_by_check == 0) && (m_bx_check == 0))
            {
                m_pubkey_gen = 1;
                m_ec_pub_key_state = EC_PUB_KEY_STATE::GEN_PUB_KEY_256;
            }
            else if ((m_bx_check == 1) && (m_by_check == 0))
            {
                m_sha_last_block_counter++;
            }
            else if ((m_bx_check == 1) && (m_by_check == 1))
            {
            	m_ec_pub_key_state = EC_PUB_KEY_STATE::EMPTY_STATE;
            }
        }
        else if (data == (CRYPTO_CSR_ECDSA_INSTRUCTION_VALID_MSK | ECDSA_INSTR_WRITE_S | CRYPTO_384_MODE))
        {
            m_cur_transfer_size = 47;
        }
        else if (data == (CRYPTO_CSR_ECDSA_INSTRUCTION_VALID_MSK | ECDSA_INSTR_WRITE_S | CRYPTO_256_MODE))
        {
            m_cur_transfer_size = 31;
        }
        else if (data == (CRYPTO_CSR_ECDSA_INSTRUCTION_VALID_MSK | ECDSA_INSTR_GEN_PUB_KEY | CRYPTO_384_MODE))
        {
            m_cur_transfer_size = 47;
            m_crypto_state = CRYPTO_STATE::CRYPTO_CALC_DONE;
        }
        else if (data == (CRYPTO_CSR_ECDSA_INSTRUCTION_VALID_MSK | ECDSA_INSTR_GEN_PUB_KEY | CRYPTO_256_MODE))
        {
            m_cur_transfer_size = 31;
            m_crypto_state = CRYPTO_STATE::CRYPTO_CALC_DONE;
        }
        else if (data == (CRYPTO_CSR_ECDSA_INSTRUCTION_VALID_MSK | ECDSA_INSTR_WRITE_E | CRYPTO_384_MODE))
        {
            m_crypto_state = CRYPTO_STATE::ACCEPT_SHA_384_DATA;
            m_cur_transfer_size = 47;
        }
        else if (data == (CRYPTO_CSR_ECDSA_INSTRUCTION_VALID_MSK | ECDSA_INSTR_WRITE_E | CRYPTO_256_MODE))
        {
            m_crypto_state = CRYPTO_STATE::ACCEPT_SHA_256_DATA;
            m_cur_transfer_size = 31;
        }
        else if (data == (CRYPTO_CSR_ECDSA_INSTRUCTION_VALID_MSK | ECDSA_INSTR_VALIDATE | CRYPTO_384_MODE))
        {
            m_crypto_state = CRYPTO_STATE::CRYPTO_CALC_DONE;
            m_ec_pub_key_state = EC_PUB_KEY_STATE::EMPTY_STATE;
            m_ec_sig_state = EC_SIG_STATE::EMPTY_STATE;
            m_cur_transfer_size = 47;
        }
        else if (data == (CRYPTO_CSR_ECDSA_INSTRUCTION_VALID_MSK | ECDSA_INSTR_VALIDATE | CRYPTO_256_MODE))
        {
            m_crypto_state = CRYPTO_STATE::CRYPTO_CALC_DONE;
            m_ec_pub_key_state = EC_PUB_KEY_STATE::EMPTY_STATE;
            m_ec_sig_state = EC_SIG_STATE::EMPTY_STATE;
            m_cur_transfer_size = 31;
        }
        else if (data == (CRYPTO_CSR_ECDSA_INSTRUCTION_VALID_MSK | ECDSA_INSTR_SIGN | CRYPTO_384_MODE))
        {
            m_crypto_state = CRYPTO_STATE::CRYPTO_CALC_DONE;
            m_ec_pub_key_state = EC_PUB_KEY_STATE::EMPTY_STATE;
            m_ec_sig_state = EC_SIG_STATE::GEN_SIG_384;
            m_cur_transfer_size = 47;
        }
        else if (data == (CRYPTO_CSR_ECDSA_INSTRUCTION_VALID_MSK | ECDSA_INSTR_SIGN | CRYPTO_256_MODE))
        {
            m_crypto_state = CRYPTO_STATE::CRYPTO_CALC_DONE;
            m_ec_pub_key_state = EC_PUB_KEY_STATE::EMPTY_STATE;
            m_ec_sig_state = EC_SIG_STATE::GEN_SIG_256;
            m_cur_transfer_size = 31;
        }
    }
    else if (addr_int == CRYPTO_ECDSA_DATA_INPUT_ADDR(m_first))
    {
        // Accept the EC constant and hashed data.
        if (m_crypto_state == CRYPTO_STATE::ACCEPT_EC_DATA && (m_ec_or_sha == EC_OR_SHA_STATE::SHA_384_AND_EC))
        {
            m_crypto_data_384[m_crypto_data_idx][m_cur_transfer_size--] = ((data >> 24) & 0xFF);
            m_crypto_data_384[m_crypto_data_idx][m_cur_transfer_size--] = ((data >> 16) & 0xFF);
            m_crypto_data_384[m_crypto_data_idx][m_cur_transfer_size--] = ((data >> 8) & 0xFF);

            if (m_cur_transfer_size == 0)
            {
            	m_crypto_data_384[m_crypto_data_idx][m_cur_transfer_size] = ((data >> 0) & 0xFF);
            }
            else
            {
                m_crypto_data_384[m_crypto_data_idx][m_cur_transfer_size--] = ((data >> 0) & 0xFF);
            }

            m_first++;

            if (m_cur_transfer_size == 0)
            {
                m_cur_transfer_size = 0;
                m_crypto_data_idx++;
                m_first = 0;

                if (m_ec_pub_key_state == EC_PUB_KEY_STATE::GEN_PUB_KEY_384)
                {
                    if (m_crypto_data_idx == m_crypto_pubkey_data_elem)
                    {
                        m_crypto_state = CRYPTO_STATE::CRYPTO_CALC_DONE;
                        m_padding_end = 1;
                    }
                }
                else
                {
                    if (m_crypto_data_idx == m_crypto_data_elem)
                    {
                        m_crypto_state = CRYPTO_STATE::CRYPTO_CALC_DONE;
                        m_padding_end = 1;
                    }
                }
            }
        }
        else if (m_crypto_state == CRYPTO_STATE::ACCEPT_EC_DATA && (m_ec_or_sha == EC_OR_SHA_STATE::SHA_256_AND_EC))
        {
            m_crypto_data_256[m_crypto_data_idx][m_cur_transfer_size--] = ((data >> 24) & 0xFF);
            m_crypto_data_256[m_crypto_data_idx][m_cur_transfer_size--] = ((data >> 16) & 0xFF);
            m_crypto_data_256[m_crypto_data_idx][m_cur_transfer_size--] = ((data >> 8) & 0xFF);

            if (m_cur_transfer_size == 0)
            {
            	m_crypto_data_256[m_crypto_data_idx][m_cur_transfer_size] = ((data >> 0) & 0xFF);
            }
            else
            {
            	m_crypto_data_256[m_crypto_data_idx][m_cur_transfer_size--] = ((data >> 0) & 0xFF);
            }

            m_first++;

            if (m_cur_transfer_size == 0)
            {
                m_cur_transfer_size = 0;
                m_crypto_data_idx++;
                m_first = 0;

                if (m_ec_pub_key_state == EC_PUB_KEY_STATE::GEN_PUB_KEY_256)
                {
                    if (m_crypto_data_idx == m_crypto_pubkey_data_elem)
                    {
                        m_crypto_state = CRYPTO_STATE::CRYPTO_CALC_DONE;
                        m_padding_end = 1;
                    }
                }
                else
                {
                    if (m_crypto_data_idx == m_crypto_data_elem)
                    {
                        m_crypto_state = CRYPTO_STATE::CRYPTO_CALC_DONE;
                        m_padding_end = 1;
                    }
                }
            }
        }
        //to handle E
        else if (m_crypto_state == CRYPTO_STATE::ACCEPT_SHA_256_DATA && (m_ec_or_sha == EC_OR_SHA_STATE::SHA_256_AND_EC))
        {
            m_first += 1;

            if (m_first == 8)
            {
        	    m_first = 0;
        	    m_crypto_state = CRYPTO_STATE::CRYPTO_CALC_DONE;
            }
        }
        else if (m_crypto_state == CRYPTO_STATE::ACCEPT_SHA_384_DATA && (m_ec_or_sha == EC_OR_SHA_STATE::SHA_384_AND_EC))
        {
            m_first += 1;

            if (m_first == 12)
            {
        	    m_first = 0;
        	    m_crypto_state = CRYPTO_STATE::CRYPTO_CALC_DONE;
            }
        }
    }
    else if (addr_int == CRYPTO_DATA_ADDR)
    {
        // The crypto mock here tracks the padding size it expect from the firmware for both sha 384 and sha 256.
        // On top of that, the mock also will detect the padding size it expect from hmac calculation.
        if (m_crypto_state == CRYPTO_STATE::ACCEPT_SHA_384_DATA)
        {
            m_cur_transfer_size -= 4;

            m_sha_data.push_back((data >> 0) & 0xFF);
            m_sha_data.push_back((data >> 8) & 0xFF);
            m_sha_data.push_back((data >> 16) & 0xFF);
            m_sha_data.push_back((data >> 24) & 0xFF);

            // The padding counter order is not important here for the crypto mock to work
            /*if ((((data & 0xff000000) == 0x80000000) ||
                ((data & 0xff0000) == 0x800000) ||
                ((data & 0xff00) == 0x8000) ||
                ((data & 0xff) == 0x80)) //&&
                //(m_end_of_block == 1)
            		//&& (m_padding_start == 0)
				//&& (m_padding_counter)
				)
            {
                m_padding_start = 1;
            }*/
            /*else*/ if (m_padding_start)
            {
                // Beginning from here, the crypto mock is checking for the correct padding size from Nios
                // If wrong padding size from Nios, m_padding_counter will not be reset and the operation fails

                if (data == 0)
                {
                    m_padding_counter++;
                }
                else if (data != 0)
                {
                    alt_u32 temp_data;
                    alt_u32 no_of_padded_bytes = 0;
                    alt_u32 no_of_padded_zeros = 0;
                    alt_u32 quotient = 0;
                    temp_data = (((data >> 24) & 0xff) | ((data >> 8) & 0xff00) | ((data << 8) & 0xff0000) | ((data << 24) & 0xff000000))/8;

                    alt_u32 processed_size = 4*(temp_data / 4);
                    alt_u32 leftover_bytes = temp_data - processed_size;
                    quotient = processed_size / CRYPTO_BLOCK_SIZE_FOR_384;
                    no_of_padded_zeros = (quotient*CRYPTO_BLOCK_SIZE_FOR_384) + CRYPTO_BLOCK_SIZE_FOR_384 - processed_size - 8;
                    no_of_padded_bytes = no_of_padded_zeros + (4 - leftover_bytes) + 4;

                    alt_u32 min_placeholder_bytes = CRYPTO_MINIMUM_LEFTOVER_BIT_384 / 8;
                    alt_u32 placeholder_bytes_check = 0;
                    alt_u32 block_size = CRYPTO_BLOCK_SIZE_FOR_384;

                    if (temp_data > block_size)
                    {
                        placeholder_bytes_check = block_size - (temp_data - (block_size * (temp_data / block_size)));
                    }
                    else if (temp_data < block_size)
                    {
                        placeholder_bytes_check = block_size - temp_data;
                    }

                    if ((placeholder_bytes_check < min_placeholder_bytes) && ((temp_data % block_size) != 0))
                    {
                        // Extend to next multiple of block size
                    	no_of_padded_zeros = (4*(placeholder_bytes_check/4) + ((block_size/4) - 1))*4;
                    	no_of_padded_bytes = no_of_padded_zeros + (4 - leftover_bytes) + 4;
                    }

                    if ((temp_data == (m_sha_data.size() - no_of_padded_bytes)) && (m_padding_counter == (no_of_padded_zeros / 4)))
                    {
                        m_padding_end = 1;
                        m_padding_start = 0;
                        m_padding_counter = 0;
                        for (alt_u32 i = 0; i < no_of_padded_bytes; i++)
                        {
                            // Actually detects the appropriate padding size from NIOS
                            // Removes the padding as OpenSSL already automatically do padding
                        	m_sha_data.pop_back();
                        }
                        //m_end_of_block = 1;
                    }
                    else
                    {

                        //if (m_padding_counter == 0)
                        //{
                            //m_padding_start = 1;
                            //m_padding_counter = 0;
                        //}
                        //else
                        //{
                            m_padding_counter = 0;
                            m_padding_start = 0;
                            //m_end_of_block = 0;
                        //}
                    }
                } // Leaving as reference to fallback to old algorithm
                /*else if ((data != 0) && (m_padding_counter == 6))
                {
                    alt_u32 temp_data;

                    temp_data = (((data >> 24) & 0xff) | ((data >> 8) & 0xff00) | ((data << 8) & 0xff0000) | ((data << 24) & 0xff000000))/8;

                    if ((temp_data == (m_sha_data.size() - 32)) && (m_padding_counter == 6))
                    {
                        m_padding_end = 1;
                        m_padding_start = 0;
                        m_padding_counter = 0;
                        alt_u32 i = 0;
                        for (i = 0; i < 8; i++)
                        {
                            // Actually detects the appropriate padding size from NIOS
                            // Removes the padding as OpenSSL already automatically do padding
                        	m_sha_data.pop_back();
                        	m_sha_data.pop_back();
                        	m_sha_data.pop_back();
                        	m_sha_data.pop_back();
                        }
                    }
                }
                else if ((data != 0) && (m_padding_counter == 14))
                {
                    alt_u32 temp_data;

                    temp_data = (((data >> 24) & 0xff) | ((data >> 8) & 0xff00) | ((data << 8) & 0xff0000) | ((data << 24) & 0xff000000))/8;

                    if ((temp_data == (m_sha_data.size() - 64)) && (m_padding_counter == 14))
                    {
                	    m_padding_end = 1;
                	    m_padding_start = 0;
                	    m_padding_counter = 0;
                	    alt_u32 i = 0;
                	    for (i = 0; i < 16; i++)
                	    {
                            m_sha_data.pop_back();
                            m_sha_data.pop_back();
                            m_sha_data.pop_back();
                            m_sha_data.pop_back();
                	    }
                    }
                }
                else if ((data != 0) && (m_padding_counter == 30))
                {
                    alt_u32 temp_data;

                    temp_data = (((data >> 24) & 0xff) | ((data >> 8) & 0xff00) | ((data << 8) & 0xff0000) | ((data << 24) & 0xff000000))/8;

                    if ((temp_data == (m_sha_data.size() - 128)) && (m_padding_counter == 30))
                    {
                	    m_padding_end = 1;
                	    m_padding_start = 0;
                	    m_padding_counter = 0;
                	    alt_u32 i = 0;
                	    for (i = 0; i < 32; i++)
                	    {
                            m_sha_data.pop_back();
                            m_sha_data.pop_back();
                            m_sha_data.pop_back();
                            m_sha_data.pop_back();
                	    }
                    }
                }
                else if ((data != 0) && (m_padding_counter != 30))
                {
                    m_padding_counter = 0;
                }
                else if ((data != 0) && (m_padding_counter != 18))
                {
                    m_padding_counter = 0;
                }
                else if ((data != 0) && (m_padding_counter != 14))
                {
                    m_padding_counter = 0;
                }
                else if ((data != 0) && (m_padding_counter != 6))
                {
                    m_padding_counter = 0;
                }*/
            }
            if ((((data & 0xff000000) == 0x80000000) ||
                ((data & 0xff0000) == 0x800000) ||
                ((data & 0xff00) == 0x8000) ||
                ((data & 0xff) == 0x80)) //&&
                //(m_end_of_block == 1)
            		&& (m_padding_start == 0)
				//&& (m_padding_counter)
				)
            {
                m_padding_start = 1;
            }

            m_end_of_block = 0;

            // the m_cur_transfer_size order is VERY important for the crypto mock to detect the sha padding
            if ((m_cur_transfer_size % 16) == 0)
            {
                m_end_of_block = 1;
            }
            /*if (m_cur_transfer_size == 80)
            {
            	m_end_of_block = 1;
            }
            if (m_cur_transfer_size == 64)
            {
            	m_end_of_block = 1;
            }
            if (m_cur_transfer_size == 32)
            {
            	m_end_of_block = 1;
            }*/
            if (m_cur_transfer_size == 0)// && (m_end_of_block != 0))
            {
                m_cur_transfer_size = 0;
                m_crypto_data_idx = 0;
                m_end_of_block = 0;

                if (m_ec_or_sha == EC_OR_SHA_STATE::SHA_384_ONLY)
                {
                    m_crypto_state = CRYPTO_STATE::CRYPTO_CALC_DONE;

                    m_sha_block_counter = 128;
                }
            }
        }
        else if (m_crypto_state == CRYPTO_STATE::ACCEPT_SHA_256_DATA)
        {
            m_cur_transfer_size -= 4;

            m_sha_data.push_back((data >> 0) & 0xFF);
            m_sha_data.push_back((data >> 8) & 0xFF);
            m_sha_data.push_back((data >> 16) & 0xFF);
            m_sha_data.push_back((data >> 24) & 0xFF);

            // The padding counter order is not important here for the crypto mock to work
            if ((((data & 0xff000000) == 0x80000000) ||
                ((data & 0xff0000) == 0x800000) ||
                ((data & 0xff00) == 0x8000) ||
                ((data & 0xff) == 0x80)) &&
                (m_end_of_block == 1))
            {
                m_padding_start = 1;
            }
            else if (m_padding_start)
            {
                if (data == 0)
                {
                    m_padding_counter++;
                }
                else if (data != 0)
                {
                    alt_u32 temp_data;
                    alt_u32 no_of_padded_bytes = 0;
                    alt_u32 no_of_padded_zeros = 0;
                    alt_u32 quotient = 0;
                    temp_data = (((data >> 24) & 0xff) | ((data >> 8) & 0xff00) | ((data << 8) & 0xff0000) | ((data << 24) & 0xff000000))/8;

                    alt_u32 processed_size = 4*(temp_data / 4);
                    alt_u32 leftover_bytes = temp_data - processed_size;
                    quotient = processed_size / CRYPTO_BLOCK_SIZE_FOR_256;
                    no_of_padded_zeros = (quotient*CRYPTO_BLOCK_SIZE_FOR_256) + CRYPTO_BLOCK_SIZE_FOR_256 - processed_size - 8;
                    no_of_padded_bytes = no_of_padded_zeros + (4 - leftover_bytes) + 4;

                    if ((temp_data == (m_sha_data.size() - no_of_padded_bytes)) && (m_padding_counter == (no_of_padded_zeros / 4)))
                    {
                        m_padding_end = 1;
                        m_padding_start = 0;
                        m_padding_counter = 0;
                        for (alt_u32 i = 0; i < no_of_padded_bytes; i++)
                        {
                            // Actually detects the appropriate padding size from NIOS
                            // Removes the padding as OpenSSL already automatically do padding
                        	m_sha_data.pop_back();
                        }
                    }
                    else
                    {
                        m_padding_counter = 0;
                    }
                }// Leaving residue to fall back to old padding algorithm
                /*else if ((data != 0) && (m_padding_counter == 6))
                {
                    alt_u32 temp_data;

                    temp_data = (((data >> 24) & 0xff) | ((data >> 8) & 0xff00) | ((data << 8) & 0xff0000) | ((data << 24) & 0xff000000))/8;

                    if ((temp_data == (m_sha_data.size() - 32)) && (m_padding_counter == 6))
                    {
                        m_padding_end = 1;
                        m_padding_start = 0;
                        m_padding_counter = 0;
                        alt_u32 i = 0;
                        for (i = 0; i < 8; i++)
                        {
                            // Actually detects the appropriate padding size from NIOS
                            // Removes the padding as OpenSSL already automatically do padding
                        	m_sha_data.pop_back();
                        	m_sha_data.pop_back();
                        	m_sha_data.pop_back();
                        	m_sha_data.pop_back();
                        }
                    }
                }
                else if ((data != 0) && (m_padding_counter == 14))
                {
                    alt_u32 temp_data;

                    temp_data = (((data >> 24) & 0xff) | ((data >> 8) & 0xff00) | ((data << 8) & 0xff0000) | ((data << 24) & 0xff000000))/8;

                    if ((temp_data == (m_sha_data.size() - 64)) && (m_padding_counter == 14))
                    {
                	    m_padding_end = 1;
                	    m_padding_start = 0;
                	    m_padding_counter = 0;
                	    alt_u32 i = 0;
                	    for (i = 0; i < 16; i++)
                	    {
                            m_sha_data.pop_back();
                            m_sha_data.pop_back();
                            m_sha_data.pop_back();
                            m_sha_data.pop_back();
                	    }
                    }
                }
                else if ((data != 0) && (m_padding_counter == 2))
                {
                    alt_u32 temp_data;

                    temp_data = (((data >> 24) & 0xff) | ((data >> 8) & 0xff00) | ((data << 8) & 0xff0000) | ((data << 24) & 0xff000000))/8;

                    if ((temp_data == (m_sha_data.size() - 16)) && (m_padding_counter == 2))
                    {
                	    m_padding_end = 1;
                	    m_padding_start = 0;
                	    m_padding_counter = 0;
                	    alt_u32 i = 0;
                	    for (i = 0; i < 4; i++)
                	    {
                            m_sha_data.pop_back();
                            m_sha_data.pop_back();
                            m_sha_data.pop_back();
                            m_sha_data.pop_back();
                	    }
                    }
                }
                else if ((data != 0) && (m_padding_counter != 6))
                {
                    m_padding_counter = 0;
                }
                else if ((data != 0) && (m_padding_counter != 14))
                {
                    m_padding_counter = 0;
                }
                else if ((data != 0) && (m_padding_counter != 2))
                {
                    m_padding_counter = 0;
                }*/
            }

            // The m_cur_transfer_size order is VERY important here for the crypto mock to detect the padding
            m_end_of_block = 0;
            if ((m_cur_transfer_size % 16) == 0)
            {
                m_end_of_block = 1;
            }
            /*if (m_cur_transfer_size == 32)
            {
            	m_end_of_block = 1;
            }
            if (m_cur_transfer_size == 16)
            {
            	m_end_of_block = 1;
            }*/
            if (m_cur_transfer_size == 0)
            {
                m_cur_transfer_size = 0;
                m_crypto_data_idx = 0;
                m_end_of_block = 1;

                if (m_ec_or_sha == EC_OR_SHA_STATE::SHA_256_ONLY)
                {
                     m_crypto_state = CRYPTO_STATE::CRYPTO_CALC_DONE;

                     m_sha_block_counter += 64;
                }
            }
        }
        else
        {
            PFR_INTERNAL_ERROR("Unhandled SHA state for write to CRYPTO_DATA_ADDR");
        }
    }
    else if (addr_int == CRYPTO_GPO_1_ADDR) // DMA operations in the mock system begins here
    {
        // This part handles DMA functionality
        if ((data & (0b1 << 26)) != 0)
        {
            // Mux dma to spi is flipped here
            m_dma_mux = 1;
            m_data_length_dma = 0;
        }
        else if ((data & (0b1 << 28)) != 0)
        {
        	// Mux dma to ufm is flipped here
            m_dma_mux = 2;
            m_data_length_dma = 0;
        }
        else if ((data & (0b1 << 26)) == 0 || (data & (0b1 << 28)) == 0)
        {
            m_dma_mux = 0;
        }
    }
    else if (addr_int == CRYPTO_DMA_CSR_START_ADDRESS)
    {
        m_crypto_calc_pass = false;
        m_ufm_or_spi_starting_address = data;
    }
    else if (addr_int == CRYPTO_DMA_CSR_DATA_TRANSFER_LENGTH)
    {
        m_data_length_dma = data;
        if ((m_data_length_dma % 128) != 0)
        {
            PFR_INTERNAL_ERROR("Illegal data size is being passed into DMA");
        }
        if ((m_data_length_dma % 4) != 0)
        {
            PFR_INTERNAL_ERROR("Data size passed into DMA is not a multiple of 4");
        }
    }
    else if (addr_int == CRYPTO_DMA_CSR_ADDRESS)
    {
        // DMA spi mux
        if (m_dma_mux == 1)
        {
            if (data == (CRYPTO_DMA_CSR_GO_MASK | 0))
            {
                m_ec_or_sha = EC_OR_SHA_STATE::SHA_256_ONLY;
                m_dma_state = DMA_STATE::ACCEPT_SHA_256_DMA_DATA;
                m_dma_go_bit = 1;
            }

            if (data == (CRYPTO_DMA_CSR_GO_MASK | 4))
            {
                m_ec_or_sha = EC_OR_SHA_STATE::SHA_384_ONLY;
                m_dma_state = DMA_STATE::ACCEPT_SHA_384_DMA_DATA;
                m_dma_go_bit = 1;
            }
        }
        // DMA ufm mux
        else if (m_dma_mux == 2)
        {
            if (data == (CRYPTO_DMA_CSR_GO_MASK | 0))
            {
                m_ec_or_sha = EC_OR_SHA_STATE::SHA_256_ONLY;
                m_dma_state = DMA_STATE::ACCEPT_SHA_256_DMA_DATA;
                m_dma_go_bit = 2;
            }

            if (data == (CRYPTO_DMA_CSR_GO_MASK | 4))
            {
                m_ec_or_sha = EC_OR_SHA_STATE::SHA_384_ONLY;
                m_dma_state = DMA_STATE::ACCEPT_SHA_384_DMA_DATA;
                m_dma_go_bit = 2;
            }
        }
        else if (m_dma_mux == 0)
        {
            m_dma_go_bit = 0;
            PFR_INTERNAL_ERROR("The DMA mux has not been flipped");
        }

        if (m_dma_go_bit == 1)
        {
            // Obtain the address in the spi flash mock based on the offset address value passed in
            alt_u32* m_spi_starting_address_ptr = (alt_u32*)(SPI_FLASH_MOCK::get()->get_spi_flash_ptr() + (m_ufm_or_spi_starting_address >> 2));
            alt_u32 data_word = 0;

            while (m_data_length_dma != 0)
            {
                // Here DMA is retrieving data from the spi flash mock
                // based on the address offset value passed in
                alt_u32 dma_data = m_spi_starting_address_ptr[data_word];

                m_sha_data.push_back((dma_data >> 0) & 0xFF);
                m_sha_data.push_back((dma_data >> 8) & 0xFF);
                m_sha_data.push_back((dma_data >> 16) & 0xFF);
                m_sha_data.push_back((dma_data >> 24) & 0xFF);

                data_word += 1;
                m_data_length_dma -= 4;
            }

            data_word = 0;
            m_padding_end = 1;
            m_crypto_state = CRYPTO_STATE::CRYPTO_CALC_DONE;
            m_dma_state = DMA_STATE::DMA_DONE;
        }
        if (m_dma_go_bit == 2)
        {
            // Obtain the address in the ufm flash mock based on the offset address value passed in
            alt_u32* m_ufm_starting_address_ptr = (alt_u32*)(UFM_MOCK::get()->get_flash_ptr() + (m_ufm_or_spi_starting_address >> 2));
            alt_u32 data_word = 0;

            while (m_data_length_dma != 0)
            {
                // Here DMA is retrieving data from the ufm flash mock
                // based on the address offset value passed in
                alt_u32 dma_data = m_ufm_starting_address_ptr[data_word];

                m_sha_data.push_back((dma_data >> 0) & 0xFF);
                m_sha_data.push_back((dma_data >> 8) & 0xFF);
                m_sha_data.push_back((dma_data >> 16) & 0xFF);
                m_sha_data.push_back((dma_data >> 24) & 0xFF);

                data_word += 1;
                m_data_length_dma -= 4;
            }

            data_word = 0;
            m_padding_end = 1;
            m_crypto_state = CRYPTO_STATE::CRYPTO_CALC_DONE;
            m_dma_state = DMA_STATE::DMA_DONE;
        }
    }
    else
    {
        PFR_INTERNAL_ERROR("Undefined handler for address");
    }
}

void CRYPTO_MOCK::reset()
{
    m_crypto_state = CRYPTO_STATE::WAIT_CRYPTO_START;
    m_data_length = 0;
    m_cur_transfer_size = 0;

    // Resize the sha data to reallocate
    m_sha_data.resize(0);

    // Clear all vectors
    for (auto it : m_crypto_data_256)
    {
        it.fill(0);
    }

    for (auto it : m_crypto_data_384)
    {
        it.fill(0);
    }

    // Clear the stored temporary data
    for (alt_u32 word_i = 0; word_i < (SHA384_LENGTH / 4); word_i++)
    {
        m_calculated_sha_384[word_i] = 0;
        m_pub_key_cx_384[word_i] = 0;
        m_pub_key_cy_384[word_i] = 0;
        m_sig_r_384[word_i] = 0;
        m_sig_s_384[word_i] = 0;
    }

    for (alt_u32 word_i = 0; word_i < (SHA256_LENGTH / 4); word_i++)
    {
        m_calculated_sha_256[word_i] = 0;
        m_pub_key_cx_256[word_i] = 0;
        m_pub_key_cy_256[word_i] = 0;
        m_sig_r_256[word_i] = 0;
        m_sig_s_256[word_i] = 0;
    }

    for (alt_u32 word_i = 0; word_i < 64; word_i++)
    {
        m_entropy_sample[word_i] = 0;
    }

    m_entropy_health_error = 0;
    m_entropy_synthetic_seed = 0;
    m_pubkey_gen = 0;
    m_sig_gen = 0;
    m_bx_check = 0;
    m_by_check = 0;
    m_sha_last_block_counter = 0;
    m_data_length = 0;
    m_data_length_dma = 0;
    m_dma_go_bit = 0;
    m_dma_mux = 2;
    m_padding_start = 0;
    m_padding_end = 0;
    m_ufm_or_spi_starting_address = 0;
    m_end_of_block = 0;
    m_first = 0;
    m_dma_done = 0;
    m_cx_verified = 0;
    m_padding_counter = 0;
    m_cur_transfer_size = 0;
    m_crypto_data_idx = 0;
    m_sha_block_counter = 0;
}

bool CRYPTO_MOCK::is_addr_in_range(void* addr)
{
    return (MEMORY_MOCK_IF::is_addr_in_range(addr, __IO_CALC_ADDRESS_NATIVE_ALT_U32(U_CRYPTO_AVMM_BRIDGE_BASE, 0), U_CRYPTO_AVMM_BRIDGE_SPAN))
            || (MEMORY_MOCK_IF::is_addr_in_range(addr, __IO_CALC_ADDRESS_NATIVE_ALT_U32(U_CRYPTO_DMA_AVMM_BRIDGE_BASE, 0), U_CRYPTO_DMA_AVMM_BRIDGE_SPAN))
            || (MEMORY_MOCK_IF::is_addr_in_range(addr, __IO_CALC_ADDRESS_NATIVE_ALT_U32(U_GPO_1_BASE, 0), U_GPO_1_SPAN));
}

// Entropy mock which provide random data.
void CRYPTO_MOCK::entropy_source_computation()
{
    // Initialises random seed
    std::srand(std::time(nullptr));

    alt_u8* m_entropy_sample_ptr = (alt_u8*) m_entropy_sample;
    for (int i = 0; i < (SHA256_LENGTH * 8); i++)
    {
        m_entropy_sample_ptr[i] = std::rand() % 256;
    }
}

// Perform all the necessary cryptographic operation.
// ECDSA 256/384 verification
// SHA 256/384 calculation
// Public key generation using custom private key
void CRYPTO_MOCK::compute_crypto_calculation()
{
    m_crypto_calc_pass = false;
    if (m_ec_or_sha == EC_OR_SHA_STATE::SHA_384_ONLY)
    {
        unsigned int md_len_384;
        unsigned char md_value_384[384];

        EVP_MD_CTX* mdctx = EVP_MD_CTX_create();
        const EVP_MD* md = EVP_sha384();

        // Initialize the SHA
        EVP_DigestInit_ex(mdctx, md, nullptr);

        // Create the SHA on the block, this creates the hash
        EVP_DigestUpdate(mdctx, m_sha_data.data(), m_sha_data.size());

        // Finalize the digest, place it in md_value_384, after calling this, cannot call evpdigestupdate anymore
        EVP_DigestFinal_ex(mdctx, md_value_384, &md_len_384);
        PFR_ASSERT(md_len_384 == SHA384_LENGTH);

        // Compare to the expected value
        alt_u8* m_calculated_sha_ptr = (alt_u8*) m_calculated_sha_384;
        for (int i = 0; i < SHA384_LENGTH; i++)
        {
            m_calculated_sha_ptr[i] = md_value_384[i];
        }

        EVP_MD_CTX_destroy(mdctx);
    }
    else if (m_ec_or_sha == EC_OR_SHA_STATE::SHA_256_ONLY)
    {
        unsigned int md_len;
        unsigned char md_value[256];

        EVP_MD_CTX* mdctx = EVP_MD_CTX_create();
        const EVP_MD* md = EVP_sha256();

        // Initialize the SHA
        EVP_DigestInit_ex(mdctx, md, nullptr);

        // Create the SHA on the block
        EVP_DigestUpdate(mdctx, m_sha_data.data(), m_sha_data.size());

        // Finalize the digest
        EVP_DigestFinal_ex(mdctx, md_value, &md_len);
        PFR_ASSERT(md_len == SHA256_LENGTH);

        // Compare to the expected value
        alt_u8* m_calculated_sha_ptr = (alt_u8*) m_calculated_sha_256;
        for (int i = 0; i < SHA256_LENGTH; i++)
        {
            m_calculated_sha_ptr[i] = md_value[i];
        }

        EVP_MD_CTX_destroy(mdctx);
    }
    else if ((m_ec_or_sha == EC_OR_SHA_STATE::SHA_384_AND_EC) && (m_ec_pub_key_state == EC_PUB_KEY_STATE::EMPTY_STATE) && (m_ec_sig_state == EC_SIG_STATE::EMPTY_STATE))
    {
        BIGNUM* bn_bx;
        BIGNUM* bn_by;
        BIGNUM* bn_r;
        BIGNUM* bn_s;
        unsigned int md_len_ec;  	//return 0;
        unsigned char md_value_ec[384];

        EVP_MD_CTX* mdctx = EVP_MD_CTX_create();
        const EVP_MD* md = EVP_sha384();

        // Initialize the SHA
        EVP_DigestInit_ex(mdctx, md, nullptr);

        // Create the SHA on the block
        EVP_DigestUpdate(mdctx, m_sha_data.data(), m_sha_data.size());

        // Finalize the digest
        EVP_DigestFinal_ex(mdctx, md_value_ec, &md_len_ec);
        PFR_ASSERT(md_len_ec == 48);
        EVP_MD_CTX_destroy(mdctx);

        // Set the public key in the EC key
        bn_bx = BN_bin2bn(m_crypto_data_384[2].data(), SHA384_LENGTH, nullptr);
        bn_by = BN_bin2bn(m_crypto_data_384[3].data(), SHA384_LENGTH, nullptr);
        bn_r = BN_bin2bn(m_crypto_data_384[7].data(), SHA384_LENGTH, nullptr);
        bn_s = BN_bin2bn(m_crypto_data_384[8].data(), SHA384_LENGTH, nullptr);

        // Set the public key components in EC_KEY
        EC_KEY* eckey = EC_KEY_new_by_curve_name(NID_secp384r1);
        EC_KEY_set_public_key_affine_coordinates(eckey, bn_bx, bn_by);

        // Set the R/S in the signature struct
        ECDSA_SIG sig;
        sig.r = bn_r;
        sig.s = bn_s;

        // Test the signature
        m_crypto_calc_pass = (ECDSA_do_verify(md_value_ec, SHA384_LENGTH, &sig, eckey) == 1);

        // Clean up
        EC_KEY_free(eckey);
        BN_free(bn_bx);
        BN_free(bn_by);
        BN_free(bn_r);
        BN_free(bn_s);
    }
    else if ((m_ec_or_sha == EC_OR_SHA_STATE::SHA_256_AND_EC) && (m_ec_pub_key_state == EC_PUB_KEY_STATE::EMPTY_STATE) && (m_ec_sig_state == EC_SIG_STATE::EMPTY_STATE))
    {
        BIGNUM* bn_bx;
        BIGNUM* bn_by;
        BIGNUM* bn_r;
        BIGNUM* bn_s;
        unsigned int md_len_ec;
        unsigned char md_value_ec[256];

        EVP_MD_CTX* mdctx = EVP_MD_CTX_create();
        const EVP_MD* md = EVP_sha256();

        // Initialize the SHA
        EVP_DigestInit_ex(mdctx, md, nullptr);

        // Create the SHA on the block
        EVP_DigestUpdate(mdctx, m_sha_data.data(), m_sha_data.size());

        // Finalize the digest
        EVP_DigestFinal_ex(mdctx, md_value_ec, &md_len_ec);
        PFR_ASSERT(md_len_ec == SHA256_LENGTH);
        EVP_MD_CTX_destroy(mdctx);

        // Set the public key in the EC key
        bn_bx = BN_bin2bn(m_crypto_data_256[2].data(), SHA256_LENGTH, nullptr);
        bn_by = BN_bin2bn(m_crypto_data_256[3].data(), SHA256_LENGTH, nullptr);
        bn_r = BN_bin2bn(m_crypto_data_256[7].data(), SHA256_LENGTH, nullptr);
        bn_s = BN_bin2bn(m_crypto_data_256[8].data(), SHA256_LENGTH, nullptr);

        // Set the public key components in EC_KEY
        EC_KEY* eckey = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
        EC_KEY_set_public_key_affine_coordinates(eckey, bn_bx, bn_by);

        // Set the R/S in the signature struct
        ECDSA_SIG sig;
        sig.r = bn_r;
        sig.s = bn_s;

        // Test the signature
        m_crypto_calc_pass = (ECDSA_do_verify(md_value_ec, SHA256_LENGTH, &sig, eckey) == 1);

        // Clean up
        EC_KEY_free(eckey);
        BN_free(bn_bx);
        BN_free(bn_by);
        BN_free(bn_r);
        BN_free(bn_s);
    }
    else if (m_ec_pub_key_state == EC_PUB_KEY_STATE::GEN_PUB_KEY_384)
    {
        // Initialise BIGNUM ctx structure and variable
        BN_CTX* key_ctx = BN_CTX_new();
        BIGNUM* bn_priv_key;

        // Converting keys to BIGNUM
        bn_priv_key = BN_bin2bn(m_crypto_data_384[5].data(), SHA384_LENGTH, nullptr);

        // Set private key into the eckey
        EC_KEY* eckey = EC_KEY_new_by_curve_name(NID_secp384r1);
        EC_KEY_set_private_key(eckey, bn_priv_key);

        // Get curve parameters
        const EC_GROUP* ec_group = EC_KEY_get0_group(eckey);

        // Create pub keys
        EC_POINT* bn_pub_key = EC_POINT_new(ec_group);
        EC_POINT_mul(ec_group, bn_pub_key, bn_priv_key, NULL, NULL, key_ctx);

        // Set pub keys in eckey
        EC_KEY_set_public_key(eckey, bn_pub_key);

        // Always remember to free the memory!
        BN_free(bn_priv_key);

        char* pub_key_char = EC_POINT_point2hex(ec_group, EC_KEY_get0_public_key(eckey), POINT_CONVERSION_UNCOMPRESSED, key_ctx);
        alt_u8* pub_key = reinterpret_cast<alt_u8*>(pub_key_char);

        alt_u8* m_pub_key_cx_ptr = (alt_u8*) m_pub_key_cx_384;
        alt_u8* m_pub_key_cy_ptr = (alt_u8*) m_pub_key_cy_384;

        unsigned char pub_key_cx_buf[SHA384_LENGTH]{};
        unsigned char pub_key_cy_buf[SHA384_LENGTH]{};

        alt_u8 ls4b = 0;
        alt_u8 ms4b = 0;
        alt_u8 nbytes = 0;

        // Converitng ASCII to integer as the key returned is in ASCII format.
        for (int i = 2; i < (2 + 96); i+=2)
        {
        	ls4b = (pub_key[i+1] > '9') ? (pub_key[i+1] - '7') : (pub_key[i+1] - '0');
        	ms4b = (pub_key[i] > '9') ? (pub_key[i] - '7') : (pub_key[i] - '0');
        	pub_key_cx_buf[nbytes] = (ms4b << 4) | (ls4b);

        	m_pub_key_cx_ptr[nbytes] = pub_key_cx_buf[nbytes];

        	nbytes++;
        	if (nbytes == SHA384_LENGTH)
        	{
        		nbytes = 0;
        	}
        }

        for (int i = 2 + 96; i < (2 + 96 + 96); i+=2)
        {
        	ls4b = (pub_key[i+1] > '9') ? (pub_key[i+1] - '7') : (pub_key[i+1] - '0');
        	ms4b = (pub_key[i] > '9') ? (pub_key[i] - '7') : (pub_key[i] - '0');
        	pub_key_cy_buf[nbytes] = (ms4b << 4) | (ls4b);

        	m_pub_key_cy_ptr[nbytes] = pub_key_cy_buf[nbytes];

        	nbytes++;
        	if (nbytes == SHA384_LENGTH)
        	{
        		nbytes = 0;
        	}
        }

        /* Cleaning up */
        EC_KEY_free(eckey);
        EC_POINT_free(bn_pub_key);
        BN_CTX_free(key_ctx);
        OPENSSL_free(pub_key_char);
    }
    else if (m_ec_pub_key_state == EC_PUB_KEY_STATE::GEN_PUB_KEY_256)
    {
        // Initialise BIGNUM ctx structure and variable
        BN_CTX* key_ctx = BN_CTX_new();
        BIGNUM* bn_priv_key;

        // Converting keys to BIGNUM
        bn_priv_key = BN_bin2bn(m_crypto_data_256[5].data(), SHA256_LENGTH, nullptr);

        // Set private key into the eckey
        EC_KEY* eckey = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
        EC_KEY_set_private_key(eckey, bn_priv_key);

        // Get curve parameters
        const EC_GROUP* ec_group = EC_KEY_get0_group(eckey);

        // Create pub keys
        EC_POINT* bn_pub_key = EC_POINT_new(ec_group);
        EC_POINT_mul(ec_group, bn_pub_key, bn_priv_key, NULL, NULL, key_ctx);

        // Set pub keys in eckey
        EC_KEY_set_public_key(eckey, bn_pub_key);

        BN_free(bn_priv_key);

        char* pub_key_char = EC_POINT_point2hex(ec_group, EC_KEY_get0_public_key(eckey), POINT_CONVERSION_UNCOMPRESSED, key_ctx);
        alt_u8* pub_key = reinterpret_cast<alt_u8*>(pub_key_char);

        alt_u8* m_pub_key_cx_ptr = (alt_u8*) m_pub_key_cx_256;
        alt_u8* m_pub_key_cy_ptr = (alt_u8*) m_pub_key_cy_256;

        unsigned char pub_key_cx_buf[SHA256_LENGTH]{};
        unsigned char pub_key_cy_buf[SHA256_LENGTH]{};

        alt_u8 ls4b = 0;
        alt_u8 ms4b = 0;
        alt_u8 nbytes = 0;

        for (int i = 2; i < (2 + 64); i+=2)
        {
        	ls4b = (pub_key[i+1] > '9') ? (pub_key[i+1] - '7') : (pub_key[i+1] - '0');
        	ms4b = (pub_key[i] > '9') ? (pub_key[i] - '7') : (pub_key[i] - '0');
        	pub_key_cx_buf[nbytes] = (ms4b << 4) | (ls4b);

        	m_pub_key_cx_ptr[nbytes] = pub_key_cx_buf[nbytes];

        	nbytes++;
        	if (nbytes == SHA256_LENGTH)
        	{
        		nbytes = 0;
        	}
        }

        for (int j = 2 + 64; j < (2 + 64 + 64); j+=2)
        {
        	ls4b = (pub_key[j+1] > '9') ? (pub_key[j+1] - '7') : (pub_key[j+1] - '0');
        	ms4b = (pub_key[j] > '9') ? (pub_key[j] - '7') : (pub_key[j] - '0');
        	pub_key_cy_buf[nbytes] = (ms4b << 4) | (ls4b);

        	m_pub_key_cy_ptr[nbytes] = pub_key_cy_buf[nbytes];

        	nbytes++;
        	if (nbytes == SHA256_LENGTH)
        	{
        		nbytes = 0;
        	}
        }

        /* Cleaning up */
        BN_CTX_free(key_ctx);
        EC_KEY_free(eckey);
        EC_POINT_free(bn_pub_key);
        OPENSSL_free(pub_key_char);
    }
    else if (m_ec_sig_state == EC_SIG_STATE::GEN_SIG_384)
    {
        // Initialise BIGNUM ctx structure and variable
        BN_CTX* key_ctx = BN_CTX_new();
    	BIGNUM* bn_priv_key;
        unsigned int md_len_ec;  	//return 0;
        unsigned char md_value_ec[384];

        // Calculate hash
        EVP_MD_CTX* mdctx = EVP_MD_CTX_create();
        const EVP_MD* md = EVP_sha384();

        // Initialize the SHA
        EVP_DigestInit_ex(mdctx, md, nullptr);

        // Create the SHA on the block
        EVP_DigestUpdate(mdctx, m_sha_data.data(), m_sha_data.size());

        // Finalize the digest
        EVP_DigestFinal_ex(mdctx, md_value_ec, &md_len_ec);
        PFR_ASSERT(md_len_ec == 48);
        EVP_MD_CTX_destroy(mdctx);

        // Generate pub keys, set private and public keys into eckey
        // Converting keys to BIGNUM
        bn_priv_key = BN_bin2bn(m_crypto_data_384[3].data(), SHA384_LENGTH, nullptr);

        // Set private key into the eckey
        EC_KEY* eckey = EC_KEY_new_by_curve_name(NID_secp384r1);
        EC_KEY_set_private_key(eckey, bn_priv_key);

        // Get curve parameters
        const EC_GROUP* ec_group = EC_KEY_get0_group(eckey);

        // Create pub keys
        EC_POINT* bn_pub_key = EC_POINT_new(ec_group);
        EC_POINT_mul(ec_group, bn_pub_key, bn_priv_key, NULL, NULL, key_ctx);

        // Set pub keys in eckey
        EC_KEY_set_public_key(eckey, bn_pub_key);

        // Always remember to free the memory!
        BN_free(bn_priv_key);

        // Generate signature
        ECDSA_SIG* ec_sig = ECDSA_do_sign(md_value_ec, SHA384_LENGTH, eckey);
        char* sig_r_hex_str = BN_bn2hex(ec_sig->r);
        char* sig_s_hex_str = BN_bn2hex(ec_sig->s);

        while ((strlen(sig_r_hex_str) != m_char_per_384_signature) || (strlen(sig_s_hex_str) != m_char_per_384_signature))
        {
            free(sig_r_hex_str);
            free(sig_s_hex_str);
            ECDSA_SIG_free(ec_sig);

            ec_sig = ECDSA_do_sign(md_value_ec, SHA384_LENGTH, eckey);

            sig_r_hex_str = BN_bn2hex(ec_sig->r);
            sig_s_hex_str = BN_bn2hex(ec_sig->s);
        }

        alt_u8* sig_r = reinterpret_cast<alt_u8*>(sig_r_hex_str);
        alt_u8* sig_s = reinterpret_cast<alt_u8*>(sig_s_hex_str);

        alt_u8* m_sig_r_384_ptr = (alt_u8*) m_sig_r_384;
        alt_u8* m_sig_s_384_ptr = (alt_u8*) m_sig_s_384;

        unsigned char sig_r_384_buf[SHA384_LENGTH]{};
        unsigned char sig_s_384_buf[SHA384_LENGTH]{};

        alt_u8 ls4b = 0;
        alt_u8 ms4b = 0;
        alt_u8 nbytes = 0;

        // Converitng ASCII to integer as the key returned is in ASCII format.
        for (alt_u32 i = 0; i < m_char_per_384_signature; i+=2)
        {
        	ls4b = (sig_r[i+1] > '9') ? (sig_r[i+1] - '7') : (sig_r[i+1] - '0');
        	ms4b = (sig_r[i] > '9') ? (sig_r[i] - '7') : (sig_r[i] - '0');
        	sig_r_384_buf[nbytes] = (ms4b << 4) | (ls4b);

        	m_sig_r_384_ptr[nbytes] = sig_r_384_buf[nbytes];

        	nbytes++;
        	if (nbytes == SHA384_LENGTH)
        	{
        		nbytes = 0;
        	}
        }

        for (alt_u32 i = 0; i < m_char_per_384_signature; i+=2)
        {
        	ls4b = (sig_s[i+1] > '9') ? (sig_s[i+1] - '7') : (sig_s[i+1] - '0');
        	ms4b = (sig_s[i] > '9') ? (sig_s[i] - '7') : (sig_s[i] - '0');
        	sig_s_384_buf[nbytes] = (ms4b << 4) | (ls4b);

        	m_sig_s_384_ptr[nbytes] = sig_s_384_buf[nbytes];

        	nbytes++;
        	if (nbytes == SHA384_LENGTH)
        	{
        		nbytes = 0;
        	}
        }

        // Clean up
        BN_CTX_free(key_ctx);
        EC_KEY_free(eckey);
        EC_POINT_free(bn_pub_key);
        ECDSA_SIG_free(ec_sig);
        free(sig_r_hex_str);
        free(sig_s_hex_str);
    }
    else if (m_ec_sig_state == EC_SIG_STATE::GEN_SIG_256)
    {
        // Initialise BIGNUM ctx structure and variable
        BN_CTX* key_ctx = BN_CTX_new();
    	BIGNUM* bn_priv_key;
        unsigned int md_len_ec;  	//return 0;
        unsigned char md_value_ec[256];

        // Calculate hash
        EVP_MD_CTX* mdctx = EVP_MD_CTX_create();
        const EVP_MD* md = EVP_sha256();

        // Initialize the SHA
        EVP_DigestInit_ex(mdctx, md, nullptr);

        // Create the SHA on the block
        EVP_DigestUpdate(mdctx, m_sha_data.data(), m_sha_data.size());

        // Finalize the digest
        EVP_DigestFinal_ex(mdctx, md_value_ec, &md_len_ec);
        PFR_ASSERT(md_len_ec == 32);
        EVP_MD_CTX_destroy(mdctx);

        // Generate pub keys, set private and public keys into eckey
        // Converting keys to BIGNUM
        bn_priv_key = BN_bin2bn(m_crypto_data_256[3].data(), SHA256_LENGTH, nullptr);

        // Set private key into the eckey
        EC_KEY* eckey = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
        EC_KEY_set_private_key(eckey, bn_priv_key);

        // Get curve parameters
        const EC_GROUP* ec_group = EC_KEY_get0_group(eckey);

        // Create pub keys
        EC_POINT* bn_pub_key = EC_POINT_new(ec_group);
        EC_POINT_mul(ec_group, bn_pub_key, bn_priv_key, NULL, NULL, key_ctx);

        // Set pub keys in eckey
        EC_KEY_set_public_key(eckey, bn_pub_key);

        // Always remember to free the memory!
        BN_free(bn_priv_key);

        // Generate signature
        ECDSA_SIG* ec_sig = ECDSA_do_sign(md_value_ec, SHA256_LENGTH, eckey);
        char* sig_r_hex_str = BN_bn2hex(ec_sig->r);
        char* sig_s_hex_str = BN_bn2hex(ec_sig->s);

        while ((strlen(sig_r_hex_str) != m_char_per_256_signature) || (strlen(sig_s_hex_str) != m_char_per_256_signature))
        {
            free(sig_r_hex_str);
            free(sig_s_hex_str);
            ECDSA_SIG_free(ec_sig);

            ec_sig = ECDSA_do_sign(md_value_ec, SHA256_LENGTH, eckey);

            sig_r_hex_str = BN_bn2hex(ec_sig->r);
            sig_s_hex_str = BN_bn2hex(ec_sig->s);
        }

        alt_u8* sig_r = reinterpret_cast<alt_u8*>(sig_r_hex_str);
        alt_u8* sig_s = reinterpret_cast<alt_u8*>(sig_s_hex_str);

        alt_u8* m_sig_r_256_ptr = (alt_u8*) m_sig_r_256;
        alt_u8* m_sig_s_256_ptr = (alt_u8*) m_sig_s_256;

        unsigned char sig_r_256_buf[SHA256_LENGTH]{};
        unsigned char sig_s_256_buf[SHA256_LENGTH]{};

        alt_u8 ls4b = 0;
        alt_u8 ms4b = 0;
        alt_u8 nbytes = 0;

        // Converitng ASCII to integer as the key returned is in ASCII format.
        for (alt_u32 i = 0; i < m_char_per_256_signature; i+=2)
        {
        	ls4b = (sig_r[i+1] > '9') ? (sig_r[i+1] - '7') : (sig_r[i+1] - '0');
        	ms4b = (sig_r[i] > '9') ? (sig_r[i] - '7') : (sig_r[i] - '0');
        	sig_r_256_buf[nbytes] = (ms4b << 4) | (ls4b);

        	m_sig_r_256_ptr[nbytes] = sig_r_256_buf[nbytes];

        	nbytes++;
        	if (nbytes == SHA256_LENGTH)
        	{
        		nbytes = 0;
        	}
        }

        for (alt_u32 i = 0; i < m_char_per_256_signature; i+=2)
        {
        	ls4b = (sig_s[i+1] > '9') ? (sig_s[i+1] - '7') : (sig_s[i+1] - '0');
        	ms4b = (sig_s[i] > '9') ? (sig_s[i] - '7') : (sig_s[i] - '0');
        	sig_s_256_buf[nbytes] = (ms4b << 4) | (ls4b);

        	m_sig_s_256_ptr[nbytes] = sig_s_256_buf[nbytes];

        	nbytes++;
        	if (nbytes == SHA256_LENGTH)
        	{
        		nbytes = 0;
        	}
        }

        // Clean up
        BN_CTX_free(key_ctx);
        EC_KEY_free(eckey);
        EC_POINT_free(bn_pub_key);
        ECDSA_SIG_free(ec_sig);
        free(sig_r_hex_str);
        free(sig_s_hex_str);
    }
}

