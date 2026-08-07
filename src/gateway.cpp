#include "security/lora_crypto.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <LoRa.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <Update.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <time.h>
// -------- PROTOTIPOS (SOLUCIONA TU ERROR) --------
void procesarMensaje(String trama);
void evaluarSemaforo();
void visualizarDatosTexto();
void actualizarSemaforoVisual(unsigned long tiempoActual);
void ejecutarOTA();
void enviarFIWARE_mTLS(bool force = false); // Nuevo prototipo

// -------- ZERO TRUST Y mTLS --------
LoRaCrypto crypto;
const uint8_t AES_KEY[LORA_AES_KEY_SIZE] = {
    0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, 0xFE, 0xDC, 0xBA,
    0x98, 0x76, 0x54, 0x32, 0x10, 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB,
    0xCD, 0xEF, 0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32, 0x10};

// --- CERTIFICADOS mTLS ---
const char *rootCACertificate = R"EOF(
-----BEGIN CERTIFICATE-----
MIICMjCCAdegAwIBAgIUNerWgY8fHwX1hLNTG/yaPoX91dowCgYIKoZIzj0EAwIw
bjELMAkGA1UEBhMCQ08xDjAMBgNVBAgMBVN0YXRlMQ0wCwYDVQQHDARDaXR5MRgw
FgYDVQQKDA9UZWxlbWV0cmlhSG9uZGExCzAJBgNVBAsMAklUMRkwFwYDVQQDDBBU
ZWxlbWV0cmlhUm9vdENBMB4XDTI2MDYwNDEyMjIwNloXDTM2MDYwMTEyMjIwNlow
bjELMAkGA1UEBhMCQ08xDjAMBgNVBAgMBVN0YXRlMQ0wCwYDVQQHDARDaXR5MRgw
FgYDVQQKDA9UZWxlbWV0cmlhSG9uZGExCzAJBgNVBAsMAklUMRkwFwYDVQQDDBBU
ZWxlbWV0cmlhUm9vdENBMFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEd3Hg1zku
D/xFSu0bSglr1m9S4/dvPR9Mwvlu0qurxFmBLaBtV9RJAcnTR14w/l4ZbqfTShCL
D+8Ijmk3iyiSbaNTMFEwHQYDVR0OBBYEFCw+H/+S21tIHGyxibjSDCtkG9elMB8G
A1UdIwQYMBaAFCw+H/+S21tIHGyxibjSDCtkG9elMA8GA1UdEwEB/wQFMAMBAf8w
CgYIKoZIzj0EAwIDSQAwRgIhAJNxRm4CQnLfapBBpgC/DGdXwiLgbS9WTuCCYOEl
pKDGAiEA0XSSIz1/nML87URu2pKEUpie+hhh77dXDVvAUhjma88=
-----END CERTIFICATE-----
)EOF";

const char *clientCertificate = R"EOF(
-----BEGIN CERTIFICATE-----
MIICJjCCAcugAwIBAgIULstR0seybZNHlw50WV/Rq3iOLC8wCgYIKoZIzj0EAwIw
bjELMAkGA1UEBhMCQ08xDjAMBgNVBAgMBVN0YXRlMQ0wCwYDVQQHDARDaXR5MRgw
FgYDVQQKDA9UZWxlbWV0cmlhSG9uZGExCzAJBgNVBAsMAklUMRkwFwYDVQQDDBBU
ZWxlbWV0cmlhUm9vdENBMB4XDTI2MDYwNDEyMjIxM1oXDTM2MDYwMTEyMjIxM1ow
czELMAkGA1UEBhMCQ08xDjAMBgNVBAgMBVN0YXRlMQ0wCwYDVQQHDARDaXR5MRgw
FgYDVQQKDA9UZWxlbWV0cmlhSG9uZGExEDAOBgNVBAsMB0RldmljZXMxGTAXBgNV
BAMMEEVTUDMyLUdhdGV3YXktMDEwWTATBgcqhkjOPQIBBggqhkjOPQMBBwNCAATM
An/ugIfEWhldguBVH+Qoq2ryJZj0q0BpUzAU6vrHBHFzlrhP3SOObyMatMurNIgS
XSupWlzeCBH+XLOGVi49o0IwQDAdBgNVHQ4EFgQUOYFuxplH2sSdfz8RcE91aF/f
H4cwHwYDVR0jBBgwFoAULD4f/5LbW0gcbLGJuNIMK2Qb16UwCgYIKoZIzj0EAwID
SQAwRgIhAIdwM4AV2RX6W7EMUvS6EJOJARvLxJ9gOCMorakODMudAiEAqaJPxeuB
9nf9DfQk+QxicXg73XIbmF+m0pNAitLpblM=
-----END CERTIFICATE-----
)EOF";

const char *clientPrivateKey = R"EOF(
-----BEGIN EC PRIVATE KEY-----
MHcCAQEEID2eCX1vnIzAHsGZ9zQXfg102OzuBRgoxg+0ydcGaVIdoAoGCCqGSM49
AwEHoUQDQgAEzAJ/7oCHxFoZXYLgVR/kKKtq8iWY9KtAaVMwFOr6xwRxc5a4T90j
jm8jGrTLqzSIEl0rqVpc3ggR/lyzhlYuPQ==
-----END EC PRIVATE KEY-----
)EOF";

const char *fiware_url_https = "https://10.162.40.214:8443/v2/entities/"
                               "urn:ngsi-ld:Motorcycle:CB300F_Node01/attrs";

// -------- WIFI Y MQTT --------
const char *ssid = "Serrato";
const char *password = "pi-cb300f";
const char *mqtt_server = "10.162.40.214";
const int mqtt_port = 1883;

const char *topic = "/ul/1234/esp32_01/attrs";
const char *topic_ota = "esp32/update";

// -------- OTA --------
const char *firmware_url = "http://10.162.40.214:5000/firmware";

// -------- LORA --------
#define SCK_LORA 12
#define MISO_LORA 13
#define MOSI_LORA 11
#define NSS_LORA 10
#define RST_LORA 9
#define DIO0_LORA 14

// -------- TFT --------
#define TFT_CS 4
#define TFT_DC 5
#define TFT_RST 8
#define TFT_SDA 7
#define TFT_SCL 6

// -------- CONFIG --------
#define COLOR_GRIS 0x3186
#define TEMP_OPTIMA 65
#define LIMITE_RPM_FRIO 3000
#define LIMITE_VEL_FRIO 3

struct DatosMotor
{
  float rpm;
  int temp;
  float extra;
  float vel;
  int estado;
  int contador;
};

enum EstadoSemaforo
{
  INACTIVO,
  ROJO,
  ADVERTENCIA,
  VERDE
};

DatosMotor miMoto = {0, 0, 0, 0, 0, 0};
EstadoSemaforo estadoArranque = INACTIVO;
bool realizarOTA = false;

int ultimoRSSI = 0;
unsigned long tiempoPrevio = 0;
unsigned long ultimoPaquete = 0;
unsigned long lastReconnectAttempt = 0;
const long intervalo = 1000;

SPIClass SPI_LORA(FSPI);
SPIClass SPI_TFT(HSPI);
Adafruit_ST7789 tft = Adafruit_ST7789(&SPI_TFT, TFT_CS, TFT_DC, TFT_RST);

WiFiClient espClient;
PubSubClient client(espClient);

// -------- WIFI --------
unsigned long lastWiFiAttempt = 0;

void setup_wifi()
{
  if (WiFi.status() == WL_CONNECTED)
    return;

  unsigned long now = millis();
  if (now - lastWiFiAttempt > 5000 || lastWiFiAttempt == 0)
  {
    lastWiFiAttempt = now;
    Serial.print("Conectando a WiFi ");
    Serial.println(ssid);
    WiFi.disconnect();
    WiFi.begin(ssid, password);
  }
}

// -------- OTA --------
void ejecutarOTA()
{
  Serial.println("Iniciando OTA...");

  WiFiClient clientHTTP;
  HTTPClient http;

  http.begin(clientHTTP, firmware_url);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK)
  {
    int contentLength = http.getSize();
    bool canBegin = Update.begin(contentLength > 0 ? contentLength : UPDATE_SIZE_UNKNOWN);

    if (canBegin)
    {
      WiFiClient *stream = http.getStreamPtr();
      size_t written = Update.writeStream(*stream);

      if (written == contentLength)
      {
        Serial.println("Firmware OK");
      }

      if (Update.end())
      {
        if (Update.isFinished())
        {
          Serial.println("OTA completa, reiniciando...");
          delay(1000);
          ESP.restart();
        }
      }
    }
  }
  else
  {
    Serial.println("Error OTA HTTP");
  }

  http.end();
}

// -------- MQTT CALLBACK --------
void callback(char *topicRecibido, byte *payload, unsigned int length)
{
  String msg;

  for (int i = 0; i < length; i++)
  {
    msg += (char)payload[i];
  }

  Serial.print("MQTT: ");
  Serial.println(msg);

  if (msg == "update")
  {
    realizarOTA = true;
  }
}

// -------- MQTT --------
boolean reconnect()
{
  if (client.connect("ESP32_Receptor_Moto"))
  {
    Serial.println("MQTT conectado");

    client.subscribe(topic_ota);

    return true;
  }
  return false;
}

// -------- SETUP --------
void setup()
{
  Serial.begin(115200);

  // Inicializar Motor Criptográfico
  crypto.begin(AES_KEY);
  Serial.println("Motor AES-256-GCM inicializado.");

  // Configurar NTP en UTC (GMT 0) para que la validación mTLS del certificado
  // (Not Before) no falle
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");

  setup_wifi();

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  // TFT
  SPI_TFT.begin(TFT_SCL, 15, TFT_SDA,
                TFT_CS); // Usamos el pin 15 como MISO "fantasma"
  tft.init(240, 320);
  tft.setRotation(0);
  tft.invertDisplay(true);
  tft.fillScreen(ST77XX_BLACK);
  tft.drawRoundRect(70, 50, 100, 220, 15, ST77XX_WHITE);

  // LORA
  SPI_LORA.begin(SCK_LORA, MISO_LORA, MOSI_LORA, -1);
  LoRa.setSPI(SPI_LORA);
  LoRa.setPins(NSS_LORA, RST_LORA, DIO0_LORA);

  if (!LoRa.begin(433E6))
  {
    while (1)
      ;
  }

  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(500E3);
  LoRa.setCodingRate4(5);
}

// -------- LOOP --------
void loop()
{
  if (realizarOTA)
  {
    realizarOTA = false;
    ejecutarOTA();
  }

  // 1. Mantener WiFi vivo de forma no bloqueante
  if (WiFi.status() != WL_CONNECTED)
  {
    setup_wifi();
  }
  else
  {
    // 2. Mantener MQTT vivo
    if (!client.connected())
    {
      unsigned long now = millis();
      if (now - lastReconnectAttempt > 5000)
      {
        lastReconnectAttempt = now;
        if (reconnect())
          lastReconnectAttempt = 0;
      }
    }
    else
    {
      client.loop();
    }
  }

  int packetSize = LoRa.parsePacket();

  if (packetSize)
  {
    Serial.print("¡Paquete LoRa recibido en Gateway! Tamaño: ");
    Serial.println(packetSize);
    // 1. Validar que el paquete sea lo suficientemente grande para contener IV
    // + TAG
    if (packetSize > LORA_AES_IV_SIZE + LORA_AES_TAG_SIZE)
    {
      uint8_t iv[LORA_AES_IV_SIZE];
      uint8_t tag[LORA_AES_TAG_SIZE];

      // 2. Extraer IV y TAG
      LoRa.readBytes(iv, LORA_AES_IV_SIZE);
      LoRa.readBytes(tag, LORA_AES_TAG_SIZE);

      // 3. Extraer Ciphertext
      int cipherLen = packetSize - LORA_AES_IV_SIZE - LORA_AES_TAG_SIZE;
      if (cipherLen <= 256)
      {
        uint8_t ciphertext[256];
        LoRa.readBytes(ciphertext, cipherLen);

        // 4. Desencriptar
        uint8_t plaintext[257];
        if (crypto.decrypt(ciphertext, cipherLen, iv, tag, plaintext))
        {
          plaintext[cipherLen] = '\0';
          String mensajeJSON = String((char *)plaintext);

          ultimoRSSI = LoRa.packetRssi();
          ultimoPaquete = millis();

          // 5. Pasar el JSON desencriptado a la lógica original
          procesarMensaje(mensajeJSON);
        }
        else
        {
          Serial.println("Error de desencriptacion LoRa (GCM Tag Mismatch)");
        }
      }
    }
  }

  unsigned long tiempoActual = millis();

  if (tiempoActual - tiempoPrevio >= intervalo)
  {
    tiempoPrevio = tiempoActual;

    if (tiempoActual - ultimoPaquete > 3000)
    {
      if (miMoto.rpm != 0 || estadoArranque != INACTIVO || miMoto.temp != 0) {
        miMoto.rpm = 0;
        miMoto.vel = 0;
        miMoto.temp = 0;
        ultimoRSSI = 0;
        evaluarSemaforo();

        if (client.connected())
        {
          String payload = "r|" + String(miMoto.rpm, 0) + "|t|" +
                           String(miMoto.temp) + "|v|" + String(miMoto.vel, 0) +
                           "|d|" + String(miMoto.extra, 2) + "|m|" +
                           String(miMoto.estado) + "|s|" + String(miMoto.contador);

          client.publish(topic, payload.c_str());
        }

        enviarFIWARE_mTLS(true);
      }
    }

    visualizarDatosTexto();
  }

  actualizarSemaforoVisual(tiempoActual);
}

// -------- TU LÓGICA ORIGINAL --------

void evaluarSemaforo()
{
  int tiempoTotalSegundos = (miMoto.estado * 60) + miMoto.contador;

  if (miMoto.rpm == 0)
    estadoArranque = INACTIVO;
  else if (miMoto.temp >= TEMP_OPTIMA && tiempoTotalSegundos >= 20)
    estadoArranque = VERDE;
  else if (miMoto.vel > LIMITE_VEL_FRIO || miMoto.rpm > LIMITE_RPM_FRIO)
    estadoArranque = ADVERTENCIA;
  else
    estadoArranque = ROJO;
}

void procesarMensaje(String trama)
{
  // Ahora la trama es un JSON que proviene de la desencriptación LoRa
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, trama);

  if (!error)
  {
    miMoto.rpm = doc["rpm"];
    miMoto.vel = doc["speed"];
    miMoto.temp = doc["temp"];
    miMoto.extra = doc["odo"];

    // Extraer minutos y segundos del string "MM:SS"
    String tiempoStr = doc["time"].as<String>();
    miMoto.estado = tiempoStr.substring(0, 2).toInt();
    miMoto.contador = tiempoStr.substring(3, 5).toInt();

    evaluarSemaforo();

    // 1. Conservamos tu publicación MQTT original
    if (client.connected())
    {
      String payload = "r|" + String(miMoto.rpm, 0) + "|t|" +
                       String(miMoto.temp) + "|v|" + String(miMoto.vel, 0) +
                       "|d|" + String(miMoto.extra, 2) + "|m|" +
                       String(miMoto.estado) + "|s|" + String(miMoto.contador);

      client.publish(topic, payload.c_str());
    }

    // 2. Ejecutamos la nueva capa mTLS hacia el proxy de FIWARE
    enviarFIWARE_mTLS();
  }
  else
  {
    Serial.print("Error leyendo JSON de la telemetría: ");
    Serial.println(error.c_str());
  }
}

// -------- NUEVA CAPA mTLS --------
void enviarFIWARE_mTLS(bool force)
{
  // Cooldown: enviar datos cada 5 segundos máximo
  static unsigned long ultimoEnvio = 0;
  if (!force && millis() - ultimoEnvio < 5000 && ultimoEnvio != 0)
    return;
  ultimoEnvio = millis();

  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("No hay WiFi. Omitiendo envio a FIWARE.");
    return;
  }

  // Verificar que la hora esté sincronizada por NTP para validar el certificado
  time_t now = time(nullptr);
  if (now < 1000000000)
  {
    Serial.println("Esperando sincronización de hora NTP para mTLS...");
    return;
  }

  // DEBUG: Imprimir hora UTC para verificar NTP
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.printf("NTP UTC: %04d-%02d-%02d %02d:%02d:%02d\n",
                timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

  // Cliente WiFi Seguro para mTLS (Zero Trust) — estático para reutilizar
  // sesión TLS
  static WiFiClientSecure secureClient;
  static bool sslConfigured = false;
  if (!sslConfigured)
  {
    secureClient.setCACert(rootCACertificate);
    secureClient.setCertificate(clientCertificate);
    secureClient.setPrivateKey(clientPrivateKey);
    sslConfigured = true;
  }

  HTTPClient http;
  http.setReuse(true);
  if (http.begin(secureClient, fiware_url_https))
  {
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Connection", "keep-alive");

    // Construir JSON en formato NGSI v2 para Orion Context Broker
    StaticJsonDocument<384> ngsi;
    ngsi["rpm"]["type"] = "Number";
    ngsi["rpm"]["value"] = miMoto.rpm;
    ngsi["speed"]["type"] = "Number";
    ngsi["speed"]["value"] = miMoto.vel;
    ngsi["temperature"]["type"] = "Number";
    ngsi["temperature"]["value"] = miMoto.temp;
    
    int tiempoTotalSegundos = (miMoto.estado * 60) + miMoto.contador;
    ngsi["time"]["type"] = "Number";
    ngsi["time"]["value"] = tiempoTotalSegundos;

    String jsonPayload;
    serializeJson(ngsi, jsonPayload);

    int httpCode = http.sendRequest("POST", jsonPayload);
    if (httpCode > 0)
    {
      Serial.printf("mTLS FIWARE OK: %d\n", httpCode);
    }
    else
    {
      Serial.printf("mTLS FIWARE Fallo: %s\n",
                    http.errorToString(httpCode).c_str());
    }
    // NO llamamos http.end() para mantener la conexión viva
  }
}

void visualizarDatosTexto()
{
  tft.setCursor(10, 10);
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.setTextSize(2);
  tft.printf("ODO: %.1f km   ", miMoto.extra);

  tft.setCursor(10, 32);
  tft.setTextColor(WiFi.status() == WL_CONNECTED ? ST77XX_GREEN : ST77XX_RED,
                   ST77XX_BLACK);
  tft.setTextSize(1);
  tft.printf("RSSI: %4d dBm | WiFi: %-3s   ", ultimoRSSI,
             WiFi.status() == WL_CONNECTED ? "OK" : "DIS");

  tft.setCursor(55, 285);
  tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK);
  tft.setTextSize(3);
  tft.printf("TMP: %02d%cC   ", miMoto.temp, (char)247);
}

void actualizarSemaforoVisual(unsigned long tiempoActual)
{

  static EstadoSemaforo anterior = (EstadoSemaforo)-1;
  static bool blink = false;

  int x = 120, r = 28;

  if (estadoArranque != anterior)
  {

    tft.fillCircle(x, 90, r, COLOR_GRIS);
    tft.fillCircle(x, 160, r, COLOR_GRIS);
    tft.fillCircle(x, 230, r, COLOR_GRIS);

    if (estadoArranque == ROJO || estadoArranque == ADVERTENCIA)
      tft.fillCircle(x, 90, r, ST77XX_RED);

    else if (estadoArranque == VERDE)
      tft.fillCircle(x, 230, r, ST77XX_GREEN);

    anterior = estadoArranque;
  }

  if (estadoArranque == ADVERTENCIA)
  {

    bool show = (tiempoActual % 1000) < 500;

    if (show != blink)
    {
      tft.fillCircle(x, 160, r, show ? ST77XX_YELLOW : COLOR_GRIS);
      blink = show;
    }
  }
}
