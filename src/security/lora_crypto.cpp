#include "lora_crypto.h"
#include <esp_system.h>
#include <esp_random.h>
#include <cstring>

LoRaCrypto::LoRaCrypto() {
    mbedtls_gcm_init(&ctx);
}

LoRaCrypto::~LoRaCrypto() {
    mbedtls_gcm_free(&ctx);
}

void LoRaCrypto::begin(const uint8_t *key) {
    memcpy(aes_key, key, LORA_AES_KEY_SIZE);
    // Inicializar el contexto GCM con la clave AES-256
    mbedtls_gcm_setkey(&ctx, MBEDTLS_CIPHER_ID_AES, aes_key, LORA_AES_KEY_SIZE * 8);
}

bool LoRaCrypto::encrypt(const uint8_t *plaintext, size_t plaintext_len, 
                         uint8_t *iv_out, uint8_t *tag_out, uint8_t *ciphertext_out) {
    
    // 1. Generar IV Aleatorio
    // ESP32 provee esp_fill_random para hardware RNG
    esp_fill_random(iv_out, LORA_AES_IV_SIZE);

    // 2. Ejecutar cifrado GCM
    int ret = mbedtls_gcm_crypt_and_tag(&ctx, MBEDTLS_GCM_ENCRYPT, plaintext_len,
                                        iv_out, LORA_AES_IV_SIZE,
                                        NULL, 0, // No usamos AAD (Additional Authenticated Data) extra
                                        plaintext, ciphertext_out,
                                        LORA_AES_TAG_SIZE, tag_out);

    if (ret != 0) {
        Serial.printf("Error encriptando: -0x%04X\n", (unsigned int)-ret);
        return false;
    }
    return true;
}

bool LoRaCrypto::decrypt(const uint8_t *ciphertext, size_t ciphertext_len,
                         const uint8_t *iv, const uint8_t *tag, uint8_t *plaintext_out) {
    
    // Ejecutar descifrado y validación del Tag GCM
    int ret = mbedtls_gcm_auth_decrypt(&ctx, ciphertext_len,
                                       iv, LORA_AES_IV_SIZE,
                                       NULL, 0,
                                       tag, LORA_AES_TAG_SIZE,
                                       ciphertext, plaintext_out);

    if (ret != 0) {
        // MBEDTLS_ERR_GCM_AUTH_FAILED = -0x0012
        Serial.printf("Error desencriptando (Posible manipulacion): -0x%04X\n", (unsigned int)-ret);
        return false;
    }
    return true;
}
