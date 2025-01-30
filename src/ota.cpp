#include "ota.h"

static uint32_t decimate(uint32_t ch11bit)
{
    // Simple divide-by-2 to discard the bit
    return ch11bit >> 1;
}

void PackUInt11ToChannels4x10(uint32_t const * const src, OTA_Channels_4x10 * const destChannels4x10)
{
    const unsigned DEST_PRECISION = 10; // number of bits for each dest, must be <SRC
    uint8_t *dest = (uint8_t *)destChannels4x10;
    *dest = 0;
    unsigned destShift = 0;
    for (unsigned ch=0; ch<4; ++ch)
    {
        // Convert to DEST_PRECISION value
        unsigned chVal = decimate(src[ch]);

        // Put the low bits in any remaining dest capacity
        *dest++ |= chVal << destShift;

        // Shift the high bits down and place them into the next dest byte
        unsigned srcBitsLeft = DEST_PRECISION - 8 + destShift;
        *dest = chVal >> (DEST_PRECISION - srcBitsLeft);
        // Next dest should be shifted up by the bits consumed
        // if destShift == 8 then everything should reset for another set
        // but this code only expects to do the 4 channels -> 5 bytes evenly
        destShift = srcBitsLeft;
    }
}

void UnpackChannels4x10ToUInt11(OTA_Channels_4x10 const * const srcChannels4x10, uint32_t * const dest)
{
    uint8_t const * const payload = (uint8_t const * const)srcChannels4x10;
    constexpr unsigned numOfChannels = 4;
    constexpr unsigned srcBits = 10;
    constexpr unsigned dstBits = 11;
    constexpr unsigned inputChannelMask = (1 << srcBits) - 1;
    constexpr unsigned precisionShift = dstBits - srcBits;

    // code from BetaFlight rx/crsf.cpp / bitpacker_unpack
    uint8_t bitsMerged = 0;
    uint32_t readValue = 0;
    unsigned readByteIndex = 0;
    for (uint8_t n = 0; n < numOfChannels; n++)
    {
        while (bitsMerged < srcBits)
        {
            uint8_t readByte = payload[readByteIndex++];
            readValue |= ((uint32_t) readByte) << bitsMerged;
            bitsMerged += 8;
        }
        //printf("rv=%x(%x) bm=%u\n", readValue, (readValue & channelMask), bitsMerged);
        dest[n] = (readValue & inputChannelMask) << precisionShift;
        readValue >>= srcBits;
        bitsMerged -= srcBits;
    }
}

void PackUInt11ToChannels4x2(const crsf_channels_t* src, uint8_t* destChannels4x2, uint8_t isHighAux) {
    // Pack the high channels (either CH5-CH8 or CH9-CH12 depending on isHighAux)
    if (isHighAux) {
        // Pack the high channel(11-bits) into a 2-bits value
        *destChannels4x2 = (CRSF_to_N(src->ch9, 4) << 0) |
                  (CRSF_to_N(src->ch10, 4) << 2) |
                  (CRSF_to_N(src->ch11, 4) << 4) |
                  (CRSF_to_N(src->ch12, 4) << 6);
    } else {
        *destChannels4x2 = (CRSF_to_N(src->ch5, 4) << 0) |
                  (CRSF_to_N(src->ch6, 4) << 2) |
                  (CRSF_to_N(src->ch7, 4) << 4) |
                  (CRSF_to_N(src->ch8, 4) << 6);
    }
}

void UnpackChannels4x2ToUInt11(uint8_t const srcChannels4x2, uint32_t * const dest, uint8_t isHighAux) {
  if (isHighAux) {
    dest[9] = N_to_CRSF((srcChannels4x2 >> 0) & 0x03, 3);
    dest[10] = N_to_CRSF((srcChannels4x2 >> 2) & 0x03, 3);
    dest[11] = N_to_CRSF((srcChannels4x2 >> 4) & 0x03, 3);
    dest[12] = N_to_CRSF((srcChannels4x2 >> 6) & 0x03, 3);
  } else {
    dest[5] = N_to_CRSF((srcChannels4x2 >> 0) & 0x03, 3);
    dest[6] = N_to_CRSF((srcChannels4x2 >> 2) & 0x03, 3);
    dest[7] = N_to_CRSF((srcChannels4x2 >> 4) & 0x03, 3);
    dest[8] = N_to_CRSF((srcChannels4x2 >> 6) & 0x03, 3);
  }
}


