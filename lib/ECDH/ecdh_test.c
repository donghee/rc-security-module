// ECDH Test
//
// by Joungil Yun (sigipus@etri.re.kr)
//
// 2024.10.

#include <ecdh.h>
#include <stdio.h>
#include <string.h>
#include "tools.h"

extern int generate_randombytes(uint8_t *obuf, size_t len);


int ecdh_test()
{
	int i;
	uint8_t tx_private_key[32] = { 0, };
	uint8_t rx_private_key[32] = { 0, };
	uint8_t tx_public_key[64] = { 0, };
	uint8_t rx_public_key[64] = { 0, };
	uint8_t compressed_tx_public_key[64] = { 0, };
	uint8_t compressed_rx_public_key[64] = { 0, };
	uint8_t decompressed_tx_public_key[64] = { 0, };
	uint8_t decompressed_rx_public_key[64] = { 0, };
	uint8_t tx_secret_key[32] =	{ 0, };
	uint8_t rx_secret_key[32] =	{ 0, };

	// random 값 생성 함수 등록
	ECC_set_rng(&generate_randombytes);

	const struct ECC_Curve_t *tx_curves;
	const struct ECC_Curve_t *rx_curves;

#if 1
	tx_curves = ECC_secp256k1();
	rx_curves = ECC_secp256k1();
#else
	tx_curves = ECC_secp256r1();
	rx_curves = ECC_secp256r1();
#endif


	// 1. Key pair generation on the TX side.
	if (!ECC_make_key(tx_public_key, tx_private_key, tx_curves))
	{
		CONSOL_PRINTF("TX: ECC_make_key() failed!!!\n");
		return -1;
	}

	CONSOL_PRINTF("[TX side's private key]: ");
	for (i = 0; i < 32; i++)
	{
			CONSOL_PRINTF("%02X", tx_private_key[i]);
	}
	CONSOL_PRINTF("\n");

	CONSOL_PRINTF("[TX side's public key]: ");
	for (i = 0; i < 64; i++)
	{
			CONSOL_PRINTF("%02X", tx_public_key[i]);
	}
	CONSOL_PRINTF("\n");

	ECC_compress_public_key(tx_public_key, compressed_tx_public_key, tx_curves);
	CONSOL_PRINTF("[Compressed TX side's public key]: ");
	for (i = 0; i < 64; i++)
	{
			CONSOL_PRINTF("%02X", compressed_tx_public_key[i]);
	}
	CONSOL_PRINTF("\n\n");

	// 2. Key pair generation on the RX side.
	if (!ECC_make_key(rx_public_key, rx_private_key, rx_curves))
	{
		CONSOL_PRINTF("RX: ECC_make_key() failed!!!\n");
		return -1;
	}

	CONSOL_PRINTF("[RX side's private key]: ");
	for (i = 0; i < 32; i++)
	{
			CONSOL_PRINTF("%02X", rx_private_key[i]);
	}
	CONSOL_PRINTF("\n");

	CONSOL_PRINTF("[RX side's public key]: ");
	for (i = 0; i < 64; i++)
	{
			CONSOL_PRINTF("%02X", rx_public_key[i]);
	}
	CONSOL_PRINTF("\n");

	ECC_compress_public_key(rx_public_key, compressed_rx_public_key, rx_curves);
	CONSOL_PRINTF("[Compressed RX side's public key]: ");
	for (i = 0; i < 64; i++)
	{
			CONSOL_PRINTF("%02X", compressed_rx_public_key[i]);
	}
	CONSOL_PRINTF("\n");

	// 3. Exchange public keys.


	CONSOL_PRINTF("\n                  --------------> Exchange public keys <--------------\n\n");



	// 4. Generate the same secret key on both sides.

	ECC_decompress_public_key(compressed_rx_public_key, decompressed_rx_public_key, tx_curves);
	CONSOL_PRINTF("[Decompressed RX side's public key]: ");
	for (i = 0; i < 64; i++)
	{
			CONSOL_PRINTF("%02X", decompressed_rx_public_key[i]);
	}
	CONSOL_PRINTF("\n");

	if (!ECC_shared_secret(decompressed_rx_public_key, tx_private_key, tx_secret_key, tx_curves))
	{
		printf("TX: ECC_shared_secret() failed !!!\n");
		return -2;
	}

	ECC_decompress_public_key(compressed_tx_public_key, decompressed_tx_public_key, rx_curves);
	CONSOL_PRINTF("[Decompressed TX side's public key]: ");
	for (i = 0; i < 64; i++)
	{
			CONSOL_PRINTF("%02X", decompressed_tx_public_key[i]);
	}
	CONSOL_PRINTF("\n\n");

	if (!ECC_shared_secret(decompressed_tx_public_key, rx_private_key, rx_secret_key, rx_curves))
	{
		printf("RX: ECC_shared_secret() failed !!!\n");
		return -2;
	}

	CONSOL_PRINTF("[TX side's generated secret key]: ");
	for (i = 0; i < 32; i++)
	{
			CONSOL_PRINTF("%02X", tx_secret_key[i]);
	}
	CONSOL_PRINTF("\n");

	CONSOL_PRINTF("[RX side's generated secret key]: ");
	for (i = 0; i < 32; i++)
	{
			CONSOL_PRINTF("%02X", rx_secret_key[i]);
	}

	CONSOL_PRINTF("\n                                  ================================================================\n");
	if (memcmp(tx_secret_key, rx_secret_key, sizeof(tx_secret_key)) != 0)
	{
		CONSOL_PRINTF("                                  The generated keys are not identical!!!\n");
		return -3;
	}
	else
	{
		CONSOL_PRINTF("                                  Confirm that the generated keys are identical.\n");
	}

	return 0;
}
// End of ecdh_test.c
