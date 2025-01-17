#pragma once

#include <stdint.h>

#define PACKED __attribute__((packed))

typedef struct {
    uint8_t raw[5]; // 4x 10-bit channels, see PackUInt11ToChannels4x10 for encoding
} PACKED OTA_Channels_4x10;

typedef struct {
  OTA_Channels_4x10 chLow;  // CH0-CH3
  OTA_Channels_4x10 chHigh; // AUX2-5 or AUX6-9
} PACKED RC_s;

void PackUInt11ToChannels4x10(uint32_t const * const src, OTA_Channels_4x10 * const destChannels4x10);
void UnpackChannels4x10ToUInt11(OTA_Channels_4x10 const * const srcChannels4x10, uint32_t * const dest);
