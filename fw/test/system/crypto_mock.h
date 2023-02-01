#ifndef INC_SYSTEM_CRYPTO_MOCK_H
#define INC_SYSTEM_CRYPTO_MOCK_H

// Standard headers
#include <memory>
#include <vector>
#include <array>

// Mock headers
#include "alt_types_mock.h"
#include "memory_mock.h"

// BSP headers
#include "pfr_sys.h"
#include "crypto.h"

class CRYPTO_MOCK : public MEMORY_MOCK_IF
{
public:
    CRYPTO_MOCK();
    virtual ~CRYPTO_MOCK();

    alt_u32 get_mem_word(void* addr) override;
    void set_mem_word(void* addr, alt_u32 data) override;

    void reset() override;

    bool is_addr_in_range(void* addr) override;

private:
    enum class ENTROPY_STATE
	{
        ENTROPY_AVAILABLE,

        EMPTY_STATE
	};

    enum class EC_PUB_KEY_STATE
    {
        GEN_PUB_KEY_256,
        GEN_PUB_KEY_384,

        EMPTY_STATE
    };

    enum class EC_SIG_STATE
    {
        GEN_SIG_256,
        GEN_SIG_384,

        EMPTY_STATE
    };

    enum class DMA_STATE
    {
        ACCEPT_SHA_256_DMA_DATA,
        ACCEPT_SHA_384_DMA_DATA,

        DMA_DONE
    };

    enum class CRYPTO_STATE
    {
        WAIT_CRYPTO_START,

        ACCEPT_SHA_384_DATA,
        ACCEPT_SHA_256_DATA,
        ACCEPT_EC_DATA,

        ACCEPT_PRIV_KEY,

        CRYPTO_CALC_DONE
    };

    enum class EC_OR_SHA_STATE
    {
        SHA_384_ONLY,
        SHA_256_ONLY,
        SHA_256_AND_EC,
        SHA_384_AND_EC
    };

    void entropy_source_computation();
    void compute_crypto_calculation();

    CRYPTO_STATE m_crypto_state;
    EC_OR_SHA_STATE m_ec_or_sha;
    DMA_STATE m_dma_state;
    EC_PUB_KEY_STATE m_ec_pub_key_state;
    EC_SIG_STATE m_ec_sig_state;
    ENTROPY_STATE m_entropy_state;

    std::vector<alt_u8> m_sha_data;
    const alt_u32 m_crypto_data_elem = 9;
    const alt_u32 m_crypto_pubkey_data_elem = 7;
    alt_u32 m_char_per_256_signature = 64;
    alt_u32 m_char_per_384_signature = 96;

    std::array<std::array<alt_u8, SHA384_LENGTH>, 9> m_crypto_data_384;
    std::array<std::array<alt_u8, SHA256_LENGTH>, 9> m_crypto_data_256;

    alt_u32 m_entropy_health_error;
    alt_u32 m_entropy_synthetic_seed;
    alt_u32 m_pubkey_gen;
    alt_u32 m_sig_gen;
    alt_u32 m_bx_check;
    alt_u32 m_by_check;
    alt_u32 m_sha_last_block_counter;
    alt_u32 m_data_length;
    alt_u32 m_data_length_dma;
    alt_u32 m_dma_go_bit;
    alt_u32 m_dma_mux;
    alt_u32 m_padding_start;
    alt_u32 m_padding_end;
    alt_u32 m_ufm_or_spi_starting_address;
    alt_u32 m_end_of_block;
    alt_u32 m_first;
    alt_u32 m_dma_done;
    alt_u32 m_cx_verified;
    alt_u32 m_padding_counter;
    alt_u32 m_cur_transfer_size;
    alt_u32 m_crypto_data_idx;
    alt_u32 m_sha_block_counter;
    alt_u32 m_calculated_sha_384[SHA384_LENGTH/4];
    alt_u32 m_calculated_sha_256[SHA256_LENGTH/4];
    alt_u32 m_pub_key_cx_384[SHA384_LENGTH/4];
    alt_u32 m_pub_key_cy_384[SHA384_LENGTH/4];
    alt_u32 m_pub_key_cx_256[SHA256_LENGTH/4];
    alt_u32 m_pub_key_cy_256[SHA256_LENGTH/4];
    alt_u32 m_sig_r_384[SHA384_LENGTH/4];
    alt_u32 m_sig_s_384[SHA384_LENGTH/4];
    alt_u32 m_sig_r_256[SHA256_LENGTH/4];
    alt_u32 m_sig_s_256[SHA256_LENGTH/4];
    alt_u32 m_entropy_sample[64];
    bool m_crypto_calc_pass;
};

#endif /* INC_SYSTEM_CRYPTO_MOCK_H */
