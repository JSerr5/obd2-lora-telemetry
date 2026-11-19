---

<h1 id="english">🏍️ End-to-End IoT Telemetry System (Honda CB300F)</h1>

> [!NOTE]
> **Proof of Concept (PoC) / Prototipo:** This repository contains the source code for an academic/demonstration prototype. No real credentials or private keys are included: Wi-Fi credentials, the LoRa AES key, and the mTLS private key appear as placeholders (`TUSSID`, `TU PASSWORD`, etc.) and must be replaced with your own.

![Dashboard IoT](images/iot%20dashboard.jpg)

This project is a comprehensive and secure solution for capturing on-board diagnostic data (OBD2) via the CAN bus on a motorcycle, enabling its transmission, processing, and real-time visualization. The architecture uses **ESP32-S3** hardware, wireless transmission over **LoRa**, **mTLS** security, a local cloud infrastructure managed by **Docker** with the **FIWARE** ecosystem, and time-series database storage.

---

## 🛠️ Architecture and Workflow

The system is divided into three main components that operate synchronously and asynchronously to maximize data reliability and security:

1. **Telemetry Node (Motorcycle - ESP32-S3):** Reads the CAN bus (OBD2 PID requests), packages the metrics (RPM, Speed, Temperature, Odometer), encrypts them with **AES-256-GCM**, and transmits them via LoRa at 433MHz.
2. **Gateway Node (Receiver - ESP32-S3):** Listens continuously via LoRa. Decrypts the information, validates the signatures, adapts it to the **NGSI v2 JSON** format, and forwards it to the cloud infrastructure using HTTPS and **mTLS**.
3. **Local Server (Docker Containers):** A complete ecosystem including **Nginx** as a reverse proxy with mTLS, **FIWARE Orion Context Broker** for context management, **FIWARE Cygnus** for subscriptions, and **InfluxDB** for storing time-series data.

### Hardware Images (PCBs)

Below are images of the nodes in operation:

| Transmitter Node (Motorcycle) | Receiver Node (Gateway) |
|:---:|:---:|
| <img src="images/transmiter%20node.jpg" width="400"> | <img src="images/reception%20node.jpg" width="400"> |
| *Capturing OBD2 parameters and securely sending via LoRa.* | *Reception, decryption, and mTLS forwarding to FIWARE.* |

**On-Bike Testing:**
<br>
<img src="images/test%20on%20bike.jpg" width="600">

---

## 🚀 System Deployment (Getting Started)

### 1. Docker Infrastructure and Services Deployment
This step brings up the database architecture, Orion, Cygnus, and the Nginx proxy.
1. Open **PowerShell** at the root of the project.
2. Generate mTLS certificates (first time only):
   ```powershell
   .\scripts\generate_certs.ps1
   ```
3. Bring up containers and services in the background:
   ```powershell
   docker-compose up -d
   ```
4. Set up the FIWARE Cygnus subscription:
   ```powershell
   powershell -ExecutionPolicy Bypass -File .\scripts\setup_subscription.ps1
   ```

### 2. Flashing the Gateway Node
1. Connect your **ESP32-S3** (Receiver) via USB.
2. Replace the placeholders in `src/gateway.cpp`: Wi-Fi (`TUSSID` / `TU PASSWORD`), the certificates (`ca.crt`, `client_esp32.crt`) and the private key (`client_esp32.key`) generated in step 1, and the `AES_KEY` (it must match the one in `src/main.cpp`).
3. Compile and flash:
   ```powershell
   pio run -e gateway_node -t upload
   ```

### 3. Flashing the Telemetry Node (Motorcycle)
1. Connect the **ESP32-S3** (Transmitter) on the motorcycle via USB.
2. Compile and flash:
   ```powershell
   pio run -e telemetry_node -t upload
   ```

### 4. Visualization Dashboard
Simply navigate to the `dashboard` directory and open the `index.html` file in your browser to view telemetry operating in real-time with a modern Glassmorphism interface.

---

## 🔒 Security Considerations
- A **Zero-Trust LoRa** principle is employed.
- Static buffers (plaintext and ciphertext) are limited to **256 bytes** to prevent memory overflows when encrypting with `mbedtls`.
- Gateway to Cloud communication is 100% shielded by **mTLS** through the reverse proxy.
- Private keys (`certs/*.key`), `.env` and compiled firmware are excluded via `.gitignore`: generate your own with `scripts/generate_certs.ps1` and never commit them.

## 🔌 Hardware Schematic (Pinout)
The system uses the **SN65HVD230 (CAN)** module and the **SX1278 (LoRa)** transceiver. The default configuration is:
- **CAN:** TX (GPIO 5), RX (GPIO 4)
- **LoRa:** SCK (GPIO 12), MISO (GPIO 13), MOSI (GPIO 11), NSS (GPIO 10), RST (GPIO 9), DIO0 (GPIO 14)

<br>
<br>
<hr>
<br>
<br>

<h1 id="español">🏍️ Sistema de Telemetría IoT de Extremo a Extremo (Honda CB300F)</h1>

> [!NOTE]
> *Este repositorio contiene el código fuente de un prototipo académico/demostrativo. No se incluyen credenciales ni claves privadas reales: el WiFi, la clave AES de LoRa y la clave privada mTLS aparecen como marcadores (`TUSSID`, `TU PASSWORD`, etc.) y deben reemplazarse por los tuyos.*

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
2. Reemplaza los marcadores en `src/gateway.cpp`: WiFi (`TUSSID` / `TU PASSWORD`), los certificados (`ca.crt`, `client_esp32.crt`) y la clave privada (`client_esp32.key`) generados en el paso 1, y la `AES_KEY` (debe ser la misma que en `src/main.cpp`).
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
- Las claves privadas (`certs/*.key`), el `.env` y el firmware compilado están excluidos por `.gitignore`: genera las tuyas con `scripts/generate_certs.ps1` y nunca las subas al repo.

## 🔌 Esquema de Hardware (Pinout)
El sistema hace uso del módulo **SN65HVD230 (CAN)** y del transceptor **SX1278 (LoRa)**. La configuración por defecto es:
- **CAN:** TX (GPIO 5), RX (GPIO 4)
- **LoRa:** SCK (GPIO 12), MISO (GPIO 13), MOSI (GPIO 11), NSS (GPIO 10), RST (GPIO 9), DIO0 (GPIO 14)
