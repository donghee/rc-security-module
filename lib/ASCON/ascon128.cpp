#include "ascon128.h"

#include <Arduino.h>

#define MAX_P_C_BYTE_LENGTH 256

Ascon128::Ascon128() {
  ASCON128x_reset(&ascon_TX);
  ASCON128x_reset(&ascon_RX);
}

Ascon128::~Ascon128() {
  // Clear sensitive data
  // memset(&ascon, 0, sizeof(ASCON_st));
}

int Ascon128::init(const uint8_t* K_, uint32_t K_len_, 
                   const uint8_t* A_, uint32_t A_len_, 
                   uint8_t *N_, size_t N_len_) {
  int result;

  // uint8_t K[16] = {0, };
  // uint8_t PP[MAX_P_C_BYTE_LENGTH] = {0, };
  // uint8_t CC[MAX_P_C_BYTE_LENGTH] = {0, };
  // uint8_t N[16] = {0, };
  // uint8_t A[MAX_A_BYTE_LENGTH] = {0, };
  // uint8_t T[16] = {0, };

  memcpy(K, K_, K_len_);
  memcpy(A, A_, A_len_);

  result = init();
  if (result < 0) {
    return -1;
  }

  return 0;
}

int Ascon128::init() {
  int result;

  result = ASCON128x_set_init_params(&ascon_TX, K, 128, A, 16, 16);

  if (result < 0) {
    return -1;
  }

  result = ASCON128x_set_init_params(&ascon_RX, K, 128, A, 16, 16);
  if (result < 0) {
    return -1;
  }

  return 0;
}

int Ascon128::encrypt(const uint8_t *plaintext, int plaintext_len, uint8_t *ciphertext) { // plaintext to data
  int result;

  result = ASCON128x_set_enc_params(&ascon_TX, (uint8_t *)plaintext, plaintext_len, N, 16); 
  if (result < 0) {
    return -1;
  }

  result = ASCON128x_enc(&ascon_TX);
  if (result < 0) {
    return -1;
  }

  memcpy(ciphertext, ascon_TX.CC, ascon_TX.CC_byte_length);

  return ascon_TX.CC_byte_length;
}

int Ascon128::decrypt(const uint8_t *ciphertext, uint8_t ciphertext_len, uint8_t *plaintext) { // ciphertext -> plaintext
  int result;

  result = ASCON128x_set_dec_params(&ascon_RX, ciphertext, ciphertext_len, N, 16, T);
  if (result < 0) {
    return -1;
  }

  result = ASCON128x_dec(&ascon_RX);
  if (result < 0) {
    return -1;
  }

  memcpy(plaintext, ascon_RX.PP, ascon_RX.PP_byte_length);

  return ascon_RX.PP_byte_length;
}
