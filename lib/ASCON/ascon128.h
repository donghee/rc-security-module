#pragma once

#include <cstdint>

extern "C"
{
#include "ascon128x.h"
#include <stddef.h>
}

class Ascon128 {
private:
  ASCON_st ascon_TX;
  ASCON_st ascon_RX;

  // ASCON
  // uint8_t K[16] = {0x14, 0x87, 0x0B, 0x99, 0x92, 0xEA, 0x89, 0x67, 0x8A, 0x1D, 0xDF, 0xD6, 0x30, 0x91, 0x8D, 0xF0};
  // uint8_t A[16] = {0, };
  // uint8_t N[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F }; // random 값 사용을 권장함!!!!!

  uint8_t K[16] = {0, };
  uint8_t A[16] = {0, };
  uint8_t N[16] = {0, };
  uint8_t T[16] = {0, };

  int initStatus = 0;

public:
  Ascon128();
  ~Ascon128();

  // const uint8_t* getCiphertext() const {
  //   return ascon.CC;
  // }
  //
  // uint32_t getCiphertextLength() const {
  //   return ascon.CC_byte_length;
  // }
  //
  // const uint8_t* getPlaintext() const {
  //   return ascon.PP;
  // }
  //
  // uint32_t getPlaintextLength() const {
  //   return ascon.PP_byte_length;
  // }
  //
  // const uint8_t* getTag() const {
  //   return ascon.T;
  // }
  //
  // uint32_t getTagLength() const {
  //   return ascon.T_byte_length;
  // }

  int init();

  int init(const uint8_t* K_, uint32_t K_len_, 
                   const uint8_t* A_, uint32_t A_len_, 
                   uint8_t *N_, size_t N_len_);

  int encrypt(const uint8_t *plaintext, int plaintext_len, uint8_t *ciphertext); // plaintext to data
  int decrypt(const uint8_t *ciphertext, uint8_t ciphertext_len, uint8_t *plaintext); // ciphertext ->  plaintext
};

