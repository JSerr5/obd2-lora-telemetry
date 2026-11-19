#include <Arduino.h>
#include "driver/twai.h"
#include <SPI.h>
#include <LoRa.h>
#include <ArduinoJson.h>
#include "security/lora_crypto.h"

// --- PINES CAN ---
static const gpio_num_t CAN_TX = GPIO_NUM_5;
static const gpio_num_t CAN_RX = GPIO_NUM_4;
const uint32_t TARGET_ID = 0x18DB33F1;

// --- PINES LORA EXACTOS ---
#define SCK_PIN   12
#define MISO_PIN  13
#define MOSI_PIN  11
#define NSS_PIN   10
#define RST_PIN   9
#define DIO0_PIN  14

SPIClass SPI_LORA(FSPI);

// --- SEGURIDAD ZERO TRUST (AES-256-GCM) ---
LoRaCrypto crypto;
// Clave pre-compartida de 256 bits (Debe coincidir con la del Gateway Receptor)
// REEMPLAZA con tu propia clave de 32 bytes (debe ser igual en emisor y gateway)
const uint8_t AES_KEY[LORA_AES_KEY_SIZE] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// --- VARIABLES DE TELEMETRÍA ---
float currentRPM = 0;
uint8_t currentSpeed = 0;
int currentOilTemp = 0;

// --- VARIABLES DE CRONÓMETRO Y ODÓMETRO ---
uint32_t tiempoArranque = 0;
bool motorEnMarcha = false;
uint32_t segundosAcumulados = 0;
uint32_t tiempoTotalDisplay = 0;
uint32_t lastSpeedTime = 0;
float tripDistanceKm = 0.0;

void setup_can() {
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX, CAN_RX, TWAI_MODE_NORMAL);
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
    
    if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
        twai_start();
        Serial.println("Bus CAN Iniciado...");
    }
}

void pedirPID(uint8_t pid) {
    twai_message_t msg = {};
    msg.identifier = TARGET_ID;
    msg.extd = true;
    msg.data_length_code = 8;
    msg.data[0] = 0x02;
    msg.data[1] = 0x01;
    msg.data[2] = pid;
    for(int i = 3; i < 8; i++) msg.data[i] = 0x00;
    twai_transmit(&msg, pdMS_TO_TICKS(5));
}

void setup() {
    Serial.begin(115200);
    while (!Serial);

    Serial.println("Iniciando Sistema Completo (CAN + LoRa + Encriptacion)...");

    // 1. Iniciar Módulo de Encriptación
    crypto.begin(AES_KEY);
    Serial.println("Motor criptográfico AES-256-GCM inicializado.");

    // 2. Iniciar CAN
    setup_can();

    // 3. Iniciar LoRa
    SPI_LORA.begin(SCK_PIN, MISO_PIN, MOSI_PIN, -1);
    LoRa.setSPI(SPI_LORA);
    LoRa.setPins(NSS_PIN, RST_PIN, DIO0_PIN);

    if (!LoRa.begin(433E6)) {
        Serial.println("¡Error de comunicación con el chip LoRa!");
        while (1); 
    }

    // Configuración para Máxima Velocidad a corta distancia
    LoRa.setSpreadingFactor(7); 
    LoRa.setSignalBandwidth(500E3); 
    LoRa.setCodingRate4(5); 
    LoRa.setTxPower(20); // Aumentar potencia a 20 (máximo) para asegurar conexión

    Serial.println("Módulo LoRa iniciado correctamente. Listo para transmitir de forma segura.");
    delay(1000);
}

void loop() {
    // 1. GESTIÓN DE PETICIONES CAN (Cada 20ms)
    static uint32_t lastRequest = 0;
    static uint8_t step = 0;
    
    if (millis() - lastRequest > 20) {
        lastRequest = millis();
        switch(step) {
            case 0: pedirPID(0x0C); break; // Petición RPM
            case 1: pedirPID(0x0D); break; // Petición Velocidad
            case 2: pedirPID(0x5C); break; // Petición Temperatura Aceite
        }
        step++;
        if(step > 2) step = 0;
    }

    // 2. ESCUCHA Y PROCESAMIENTO DE DATOS CAN
    twai_message_t rx_msg;
    if (twai_receive(&rx_msg, 0) == ESP_OK) {
        if (rx_msg.data[1] == 0x41) {
            uint8_t pid = rx_msg.data[2];
            switch(pid) {
                case 0x0C: { // Cálculo de RPM y Cronómetro de Uso
                    currentRPM = ((rx_msg.data[3] * 256.0) + rx_msg.data[4]) / 4.0;
                    if (currentRPM > 0) {
                        if (!motorEnMarcha) {
                            motorEnMarcha = true;
                            tiempoArranque = millis();
                        }
                        tiempoTotalDisplay = segundosAcumulados + ((millis() - tiempoArranque) / 1000);
                    } else {
                        if (motorEnMarcha) {
                            motorEnMarcha = false;
                            segundosAcumulados += (millis() - tiempoArranque) / 1000;
                        }
                        tiempoTotalDisplay = segundosAcumulados;
                    }
                    break;
                }
                case 0x0D: { // Cálculo de Velocidad y Odómetro
                    currentSpeed = rx_msg.data[3];
                    uint32_t currentTime = millis();
                    if (lastSpeedTime > 0) {
                        float hoursPassed = (currentTime - lastSpeedTime) / 3600000.0;
                        tripDistanceKm += currentSpeed * hoursPassed;
                    }
                    lastSpeedTime = currentTime;
                    break;
                }
                case 0x5C: { // Cálculo de Temperatura
                    currentOilTemp = rx_msg.data[3] - 40;
                    break;
                }
            }
        }
    }

    // 3. EMPAQUETADO JSON Y TRANSMISIÓN LORA ENCRIPTADA (Cada 1000ms)
    static uint32_t lastLoraSend = 0;
    if (millis() - lastLoraSend >= 1000) {
        lastLoraSend = millis();

        int minutos = tiempoTotalDisplay / 60;
        int segundos = tiempoTotalDisplay % 60;
        char tiempoStr[10];
        snprintf(tiempoStr, sizeof(tiempoStr), "%02d:%02d", minutos, segundos);

        // a. Empaquetar datos en JSON
        StaticJsonDocument<256> doc;
        doc["rpm"] = currentRPM;
        doc["speed"] = currentSpeed;
        doc["temp"] = currentOilTemp;
        doc["odo"] = tripDistanceKm;
        doc["time"] = tiempoStr;

        char jsonBuffer[256];
        size_t jsonLen = serializeJson(doc, jsonBuffer);

        // b. Preparar buffers para la encriptación
        uint8_t iv[LORA_AES_IV_SIZE];
        uint8_t tag[LORA_AES_TAG_SIZE];
        uint8_t ciphertext[256];

        // c. Ejecutar Encriptación AES-256-GCM
        if (crypto.encrypt((const uint8_t*)jsonBuffer, jsonLen, iv, tag, ciphertext)) {
            
            // d. Transmitir por LoRa: [IV (12)] + [TAG (16)] + [CIPHERTEXT]
            LoRa.beginPacket();
            LoRa.write(iv, LORA_AES_IV_SIZE);
            LoRa.write(tag, LORA_AES_TAG_SIZE);
            LoRa.write(ciphertext, jsonLen);
            LoRa.endPacket();

            Serial.println("==============================================");
            Serial.printf("JSON Original (%d bytes): %s\n", jsonLen, jsonBuffer);
            Serial.printf("Transmision Segura OK: IV(12) + TAG(16) + CIPHERTEXT(%d)\n", jsonLen);
            Serial.println("==============================================");
        } else {
            Serial.println("Error fatal: No se pudo encriptar el payload.");
        }
    }
}
