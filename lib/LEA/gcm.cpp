#include "gcm.h"

#include <fstream>

#include <Arduino.h>
extern HardwareSerial DebugSerial;

GCM::GCM()
{
}

void GCM::increment_nonce_counter(uint8_t *nonce)
{
	int i;
    for (i = 15; i >= 0; --i)
    {
        if (++nonce[i] != 0)
        {
            break;
        }
    }
}

// by Joungil Yun (2025.02.05.)
void increase_nonce_counter_up_to_32bits_increment(uint8_t *nonce, uint32_t increment)
{
	int i;
	uint32_t carry = increment;
	uint32_t temp;

    for (i = 15; i >= 0; --i)
    {
    	temp = nonce[i] + carry;
    	nonce[i] = (uint8_t)temp;
    	carry = temp >> 8;
        if (carry == 0)
        {
            break;
        }
    }
}

uint32_t GCM::encryption_time()
{
    delta[1] = stop[1] - start[1];
    return delta[1];
}

uint32_t GCM::decryption_time()
{
    delta[2] = stop[2] - start[2];
    return delta[2];
}

int GCM::init(uint8_t *K_, size_t K_len_, uint8_t *A_, size_t A_len_, uint8_t *N_, size_t N_len_)
{
    int result;

    // Measurement of lea encryption and decryption time
    if (ARM_CM_DWT_CTRL != 0) 			// See if DWT is available
    {
		ARM_CM_DEMCR      |= 1 << 24;	// Set bit 24
		ARM_CM_DWT_CYCCNT  = 0;
		ARM_CM_DWT_CTRL   |= 1 << 0;	// Set bit 0
    }

    memcpy(K, K_, K_len_);
    memcpy(A, A_, A_len_);
    memcpy(N, N_, N_len_);

    // 1. 바이딩 초기화 단계 키 교환 과정에서 Nonce 초기값도 암호화된 방식으로 함께 공유 (연결이 끊겨 리셋 될 때마다 수행되어야 함)
	COUNTER_TX = 0; // 초기화
	COUNTER_RX = 0; // 초기화 (COUNTER_TX와 동일한 값으로)
	initStatus = 0;
  COUNTER_4b = 0;

    result = init();
    if (result < 0) {
        return -1;
    }

    return 0;
}

int GCM::init()
{
    int result;

    // Kbits= 128, Abytes=16, Tbits = 32
    // TODO: To reduce the packet size, the bit size of T must be reduced. 8 * 16 = 128
    // if (GCM4LEA_set_init_params(&gcm_TX, K, 128, A, 16, 32))
    // Tbits = 16 for nonce sync, so gcm_TX.T is 2 bytes
    start[0] = ARM_CM_DWT_CYCCNT;
    // K 16 bytes = 128 bits, A 16 bytes, T 16 bits
    result = GCM4LEA_set_init_params(&gcm_TX, K, 128, A, 16, 16);
    stop[0] = ARM_CM_DWT_CYCCNT;

    if (result < 0) {
        return -1;
    }

    // TODO: delete other gcm_TX
    // if (GCM4LEA_set_init_params(&gcm_RX, K, 128, A, 16, 32))
    // Tbits = 16 for nonce sync, so gcm_RX.T is 2 bytes
    result = GCM4LEA_set_init_params(&gcm_RX, K, 128, A, 16, 16);
    if (result < 0) {
        return -1;
    }

    return 0;
}

// TX
int GCM::encrypt(const uint8_t *plaintext, int plaintext_len, uint8_t *ciphertext) // plaintext to data
{
    int result;

    // result = GCM4LEA_set_enc_params(&gcm_TX, (uint8_t *)plaintext, plaintext_len, N, 12);
    // by Joungil Yun (2025.02.05.)
    result = GCM4LEA_set_enc_params(&gcm_TX, (uint8_t *)plaintext, plaintext_len, N_GCM, 12); // N 16바이트 vs. N_GCM 12바이트
    if (result < 0) {
        return -1;
    }

    start[1] = ARM_CM_DWT_CYCCNT;
    result =  GCM4LEA_enc(&gcm_TX);
    stop[1] = ARM_CM_DWT_CYCCNT;
    if (result < 0) {
        return -1;
    }

    // 12 bits Packet Counter added by RC security module
    increment_nonce_counter(N);

    // 2025. 2  Print TX Nonce
    // DebugSerial.print("TX COUNTER: ");
    // DebugSerial.print(COUNTER_TX);
    // DebugSerial.println("");
    // DebugSerial.print("TX Nonce: ");
    // for (int i = 0; i < 16; i++)
    // {
    //     DebugSerial.print(N[i], HEX);
    //     DebugSerial.print(" ");
    // }
    // DebugSerial.println("");

    ciphertext[0] = (uint8_t)(COUNTER_TX >> 8); // 2 bytes
    ciphertext[1] = (uint8_t)COUNTER_TX;
    memcpy((uint8_t *)ciphertext + 2, gcm_TX.T, 2);
    memcpy((uint8_t *)ciphertext + 4, gcm_TX.CC, plaintext_len);

    // counter up
    COUNTER_TX = (COUNTER_TX + 1) % 4096;
    return plaintext_len + LEA_ADD_PACKET_SIZE; // ciphertext length. COUNTER(2) + T(2)
}

// RX
int GCM::decrypt(const uint8_t *ciphertext, uint8_t ciphertext_len, uint8_t *plaintext) // ciphertext -> plaintext
{
    int result;
    uint32_t plaintext_len = ciphertext_len - LEA_ADD_PACKET_SIZE;

    // 4 bits Packet Counter added by RF transmit module
    COUNTER_4b_new = ciphertext[0] >> 4; // 4 bits
    COUNTER_4b_gap = (COUNTER_4b_new - COUNTER_4b + 16) % 16;

    // 12 bits Packet Counter added by RC security module
    COUNTER_RX_new = ((ciphertext[0] & 0x0F) << 8) | ciphertext[1]; // 12 bits
    COUNTER_RX_gap = (COUNTER_RX_new - COUNTER_RX + 4096) % 4096;

    // [Note] If COUNTER_RX_gap = 0, it can be determined as transmission and reception immediately after initialization.
    // [Note] If COUNTER_RX_gap = 1, it can be determined as normal transmission and reception with an increment of 1 after the initialization phase.
    // [Note] If COUNTER_RX_gap is a large value, it can be determined that there was a long signal loss or an abnormal condition, and the initialization process should be retried.
    // DONGHEE: What is the initStatus(internal_status) != 0 ?
    if(COUNTER_RX_gap == 0 && COUNTER_4b_gap == 0 && initStatus != 0)
    {
        // Activate FAIL SAFE mode!!!!!!!!!!!!!!!!!!!!!!
        // 재전송 공격 가능성 추정
        initStatus = -1;
        return -1;
    }

    // if (COUNTER_RX_gap < 3000) // 200은 적당히 작은 값으로 변경 (실험 후)
    if((COUNTER_RX_gap < 3000 && initStatus == 0) || (COUNTER_RX_gap < 500 && initStatus != 0))
    {
      // for(int i = 0; i < COUNTER_RX_gap; i++)
      // {
      //   increment_nonce_counter(N);
      // }
      // by Joungil Yun (2025.02.05.)
      increase_nonce_counter_up_to_32bits_increment(N, COUNTER_RX_gap);

      COUNTER_RX = COUNTER_RX_new;
      COUNTER_4b = COUNTER_4b_new;

      initStatus = 1;
    }
    else
    {
      // TODO
      // 초기화, 비정상적인 상황에 대한 예외처리
      initStatus = -1;
      // return -1; ???
    }

    // 2025. 2  Print RX Nonce
    DebugSerial.print("RX COUNTER: ");
    DebugSerial.print(COUNTER_RX);
    DebugSerial.println("");
    DebugSerial.print("RX Nonce: ");
    for (int i = 0; i < 16; i++)
    {
        DebugSerial.print(N[i], HEX);
        DebugSerial.print(" ");
    }
    DebugSerial.println("");

    // Tbits = 16 for nonce sync, so ciphertext + 2 is pointer of gcm_RX.T
    // result =  GCM4LEA_set_dec_params(&gcm_RX, ciphertext + 4, plaintext_len, N, 12, ciphertext + 2);
    // by Joungil Yun (2025.02.05.)
    result =  GCM4LEA_set_dec_params(&gcm_RX, ciphertext + 4, plaintext_len, N_GCM, 12, ciphertext + 2); //N 16바이트 vs. N_GCM 12바이트

    if (result < 0) {
        return -1;
    }

    start[2] = ARM_CM_DWT_CYCCNT;
    result = GCM4LEA_dec(&gcm_RX);
    stop[2] = ARM_CM_DWT_CYCCNT;
    if (result < 0) {
        return -1;
    }

    memcpy((uint8_t *)plaintext, (uint8_t *)gcm_RX.PP, plaintext_len);

    return plaintext_len;
}
