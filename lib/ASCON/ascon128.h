extern "C"
{
#include "ascon128x.h"
#include <stddef.h>
}

class Ascon128 {
private:
  static const int RATE = 8;
  static const int CAPACITY = 8;
  static const int ROUNDS_A = 12;
  static const int ROUNDS_B = 6;

  uint64_t state[5];

  ASCON_st ascon;
public:
  Ascon128();
  ~Ascon128();

  const uint8_t* getCiphertext() const {
    return ascon.CC;
  }

  uint32_t getCiphertextLength() const {
    return ascon.CC_byte_length;
  }

  const uint8_t* getPlaintext() const {
    return ascon.PP;
  }

  uint32_t getPlaintextLength() const {
    return ascon.PP_byte_length;
  }

  const uint8_t* getTag() const {
    return ascon.T;
  }

  uint32_t getTagLength() const {
    return ascon.T_byte_length;
  }

  int initialize(const uint8_t* key, uint32_t key_bit_length, 
                 const uint8_t* a, uint32_t a_byte_length, 
                 uint32_t tag_bit_length);

  int encrypt(const uint8_t* plaintext, uint32_t plaintext_length, 
                      const uint8_t* nonce, uint32_t nonce_length);
 
  int decrypt(const uint8_t* ciphertext, uint32_t ciphertext_length, 
                      const uint8_t* nonce, uint32_t nonce_length, 
                      const uint8_t* tag);
};

