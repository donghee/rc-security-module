#include "rsa.h"

void ECDH::init()
{
  	ECC_set_rng(&generate_randombytes);
	  tx_curves = ECC_secp256k1();
	if (!ECC_make_key(tx_public_key, tx_private_key, tx_curves))
	{
		CONSOL_PRINTF("TX: ECC_make_key() failed!!!\n");
		return -1;
	}


}

void ECDH::deinit()
{
}

int ECDH::generate_key(const char *pers, size_t pers_len)
{
    int ret = 1;

    return ret;
}

int ECDH::export_pubkey(unsigned char *pubkey, size_t *pubkey_len)
{
    int ret = 1;
    return ret;
}


int ECDH::encrypt(const unsigned char *input, size_t len, unsigned char *output)
{
    int ret = 1;

	if (!ECC_shared_secret(decompressed_rx_public_key, tx_private_key, tx_secret_key, tx_curves))

    return ret;
}

int ECDH::encrypt(const unsigned char *pubkey, size_t pubkey_len, const unsigned char *input, size_t len, unsigned char *output)
{
    int ret = 1;
    int exit_code = MBEDTLS_EXIT_FAILURE;
	if (!ECC_shared_secret(decompressed_tx_public_key, rx_private_key, rx_secret_key, rx_curves))

    return ret;
}


int ECDH::decrypt(const unsigned char *ciphertext, unsigned char *decryptedtext, size_t *olen, size_t max_len)
{
    int ret = 1;

	ECC_decompress_public_key(compressed_tx_public_key, decompressed_tx_public_key, rx_curves);
    return ret;
}
