#ifndef LORA_CRYPTO_H
#define LORA_CRYPTO_H

#include <Arduino.h>
#include "mbedtls/gcm.h"

// 32 bytes = 256 bits para AES-256
#define LORA_AES_KEY_SIZE 32
// 12 bytes = 96 bits es el estándar recomendado para IV en GCM
#define LORA_AES_IV_SIZE  12
// 16 bytes = 128 bits para el Authentication Tag en GCM
#define LORA_AES_TAG_SIZE 16

class LoRaCrypto {
private:
    mbedtls_gcm_context ctx;
    uint8_t aes_key[LORA_AES_KEY_SIZE];

public:
    LoRaCrypto();
    ~LoRaCrypto();

    /**
     * @brief Inicializa el módulo criptográfico con una clave estática
     * @param key Array de 32 bytes con la clave AES-256
     */
    void begin(const uint8_t *key);

    /**
     * @brief Encripta el payload usando AES-256-GCM.
     * @param plaintext Buffer con el texto/JSON a encriptar
     * @param plaintext_len Longitud del texto a encriptar
     * @param iv_out Buffer donde se guardará el IV generado aleatoriamente (debe tener tamaño LORA_AES_IV_SIZE)
     * @param tag_out Buffer donde se guardará el Tag de autenticación (debe tener tamaño LORA_AES_TAG_SIZE)
     * @param ciphertext_out Buffer de salida para el texto encriptado (mismo tamaño que plaintext_len)
     * @return true si la encriptación fue exitosa
     */
    bool encrypt(const uint8_t *plaintext, size_t plaintext_len, 
                 uint8_t *iv_out, uint8_t *tag_out, uint8_t *ciphertext_out);

    /**
     * @brief Desencripta un payload validando el TAG y usando el IV.
     * @param ciphertext Buffer encriptado recibido
     * @param ciphertext_len Longitud del texto encriptado
     * @param iv Buffer con el IV recibido (LORA_AES_IV_SIZE)
     * @param tag Buffer con el Tag recibido (LORA_AES_TAG_SIZE)
     * @param plaintext_out Buffer de salida para el texto claro (mismo tamaño que ciphertext_len)
     * @return true si la desencriptación fue exitosa Y el TAG es válido (no fue manipulado)
     */
    bool decrypt(const uint8_t *ciphertext, size_t ciphertext_len,
                 const uint8_t *iv, const uint8_t *tag, uint8_t *plaintext_out);
};

#endif // LORA_CRYPTO_H
