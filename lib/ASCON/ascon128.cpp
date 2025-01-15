#include "ascon128.h"

Ascon128::Ascon128() {
  ASCON128x_reset(&ascon);
}

Ascon128::~Ascon128() {
  // Clear sensitive data
  // memset(&ascon, 0, sizeof(ASCON_st));
}

int Ascon128::initialize(const uint8_t* key, uint32_t key_bit_length, 
                         const uint8_t* a, uint32_t a_byte_length, 
                         uint32_t tag_bit_length) {
  return ASCON128x_set_init_params(&ascon, key, key_bit_length, 
                                   a, a_byte_length, 
                                   tag_bit_length);
}

int Ascon128::encrypt(const uint8_t* plaintext, uint32_t plaintext_length, 
                      const uint8_t* nonce, uint32_t nonce_length) {
  int result = ASCON128x_set_enc_params(&ascon, plaintext, plaintext_length, 
                                        nonce, nonce_length);
  if (result != 0) return result;

  return ASCON128x_enc(&ascon);
}

int Ascon128::decrypt(const uint8_t* ciphertext, uint32_t ciphertext_length, 
                      const uint8_t* nonce, uint32_t nonce_length, 
                      const uint8_t* tag) {
  int result = ASCON128x_set_dec_params(&ascon, ciphertext, ciphertext_length, 
                                        nonce, nonce_length, tag);
  if (result != 0) return result;

  return ASCON128x_dec(&ascon);
}

