#pragma once

#include <stdint.h>
#include "crsf_protocol.h"

#define PACKED __attribute__((packed))

typedef struct {
    uint8_t raw[5]; // 4x 10-bit channels, see PackUInt11ToChannels4x10 for encoding
} PACKED OTA_Channels_4x10;

typedef struct {
  OTA_Channels_4x10 chLow;  // CH0-CH3
  OTA_Channels_4x10 chHigh; // AUX2-5 or AUX6-9
} PACKED RC_Channels_t;

typedef struct {
  uint8_t packetType: 2,
          free: 4,
          isHighAux: 1, // true if chHigh are AUX6-10
          ch4: 1;   // AUX1, included up here so ch0 starts on a byte boundary
  uint8_t raw[10];
} PACKED RC_Channels_Encrypted_t;

void PackUInt11ToChannels4x10(uint32_t const * const src, OTA_Channels_4x10 * const destChannels4x10);
void UnpackChannels4x10ToUInt11(OTA_Channels_4x10 const * const srcChannels4x10, uint32_t * const dest);
void PackUInt11ToChannels4x2(const crsf_channels_t* src, uint8_t* destChannels4x2, uint8_t isHighAux);
void UnpackChannels4x2ToUInt11(uint8_t const srcChannels4x2, uint32_t * const dest, uint8_t isHighAux);
 
