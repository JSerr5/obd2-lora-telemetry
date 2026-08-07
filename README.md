# 🏍️ Sistema de Telemetría IoT de Extremo a Extremo (Honda CB300F)

> [!NOTE]
> **Proof of Concept (PoC) / Prototipo:** This repository contains the source code for an academic/demonstration prototype. The hardcoded credentials, keys, and certificates present in the codebase were generated exclusively for a temporary, isolated local environment and are not used in any production system.
> 
> *Este repositorio contiene el código fuente de un prototipo académico/demostrativo. Las credenciales y certificados incluidos en el código fueron generados exclusivamente para un entorno local temporal y aislado, y no se utilizan en ningún sistema en producción.*

![Dashboard IoT](images/iot%20dashboard.jpg)

Este proyecto es una solución integral y segura para la captura de datos de diagnóstico (OBD2) vía bus CAN en una motocicleta y su transmisión, procesamiento y visualización en tiempo real. La arquitectura utiliza hardware **ESP32-S3**, transmisión inalámbrica por **LoRa**, seguridad **mTLS**, infraestructura cloud local gestionada por **Docker** con el ecosistema **FIWARE**, y almacenamiento en bases de datos de series temporales.

---

## 🛠️ Arquitectura y Flujo de Trabajo

El sistema está dividido en tres componentes principales que operan de forma sincronizada y asíncrona para maximizar la fiabilidad y seguridad de los datos:

1. **Nodo Telemetría (Moto - ESP32-S3):** Lee el bus CAN (peticiones PID OBD2), empaqueta las métricas (RPM, Velocidad, Temperatura, Odómetro), las cifra con **AES-256-GCM** y las transmite vía LoRa a 433MHz.
2. **Nodo Gateway (Receptor - ESP32-S3):** Escucha ininterrumpidamente a través de LoRa. Descifra la información, valida las firmas, la adapta al formato **JSON NGSI v2**, y la envía a la infraestructura en la nube usando HTTPS y **mTLS**.
3. **Servidor Local (Contenedores Docker):** Un ecosistema completo que incluye **Nginx** como proxy reverso con mTLS, **FIWARE Orion Context Broker** para gestión del contexto, **FIWARE Cygnus** para suscripciones, e **InfluxDB** para almacenar las series temporales de datos.

### Imágenes del Hardware (PCBs)

A continuación, se presentan imágenes de los nodos en funcionamiento:

| Nodo Transmisor (Moto) | Nodo Receptor (Gateway) |
|:---:|:---:|
| <img src="images/transmiter%20node.jpg" width="400"> | <img src="images/reception%20node.jpg" width="400"> |
| *Captura de los parámetros OBD2 y envío seguro vía LoRa.* | *Recepción, descifrado y reenvío mTLS hacia FIWARE.* |

**Prueba en la Motocicleta:**
<br>
<img src="images/test%20on%20bike.jpg" width="600">

---

## 🚀 Despliegue del Sistema (Puesta en Marcha)

### 1. Despliegue de la Infraestructura y Servicios Docker
Este paso levanta la arquitectura de bases de datos, Orion, Cygnus y el proxy Nginx.
1. Abre **PowerShell** en la raíz del proyecto.
2. Genera los certificados mTLS (solo la primera vez):
   ```powershell
   .\scripts\generate_certs.ps1
   ```
3. Levanta los contenedores y servicios en segundo plano:
   ```powershell
   docker-compose up -d
   ```
4. Configura la suscripción a FIWARE Cygnus:
   ```powershell
   powershell -ExecutionPolicy Bypass -File .\scripts\setup_subscription.ps1
   ```

### 2. Flasheo del Nodo Gateway
1. Conecta tu **ESP32-S3** (Receptor) por USB.
2. Configura las credenciales (SSID y Password) y certificados (`ca.crt`, `client_esp32.crt` y `client_esp32.key`) dentro del archivo `src/gateway.cpp`.
3. Compila y flashea:
   ```powershell
   pio run -e gateway_node -t upload
   ```

### 3. Flasheo del Nodo de Telemetría (Moto)
1. Conecta el **ESP32-S3** (Transmisor) de la moto por USB.
2. Compila y flashea:
   ```powershell
   pio run -e telemetry_node -t upload
   ```

### 4. Dashboard de Visualización
Simplemente entra al directorio `dashboard` y abre el archivo `index.html` en tu navegador para ver la telemetría operando en tiempo real con una moderna interfaz Glassmorphism.

---

## 🔒 Consideraciones de Seguridad
- Se utiliza un principio de **Zero-Trust LoRa**.
- Los buffers estáticos (texto plano y ciphertext) están limitados a **256 bytes** para prevenir desbordamientos de memoria al cifrar con `mbedtls`.
- Comunicación Gateway a Nube 100% blindada por **mTLS** a través del proxy reverso.

## 🔌 Esquema de Hardware (Pinout)
El sistema hace uso del módulo **SN65HVD230 (CAN)** y del transceptor **SX1278 (LoRa)**. La configuración por defecto es:
- **CAN:** TX (GPIO 5), RX (GPIO 4)
- **LoRa:** SCK (GPIO 12), MISO (GPIO 13), MOSI (GPIO 11), NSS (GPIO 10), RST (GPIO 9), DIO0 (GPIO 14)
