# n8n-thingsboard-AI

Sistema IoT que integra un microcontrolador ESP32 con ThingsBoard y n8n para monitoreo de sensores y disparo automático de alertas mediante webhooks.

[![PlatformIO](https://img.shields.io/badge/PlatformIO-ESP32-orange?logo=platformio)](https://platformio.org/)
[![ESP32](https://img.shields.io/badge/ESP32-DevKit-blue?logo=espressif)](https://www.espressif.com/en/products/socs/esp32)
[![ThingsBoard](https://img.shields.io/badge/ThingsBoard-IoT_Platform-green?logo=thingsboard)](https://thingsboard.io/)
[![n8n](https://img.shields.io/badge/n8n-Workflow_Automation-EA4B71?logo=n8n)](https://n8n.io/)
[![Arduino](https://img.shields.io/badge/Framework-Arduino-00979D?logo=arduino)](https://www.arduino.cc/)

## Descripcion

Firmware para ESP32 que:

- Envía telemetría periódica (temperatura, humedad, uptime) cada 5 segundos a ThingsBoard vía MQTT.
- Detecta la pulsación de un botón físico y dispara una alerta (`alarm=1`) que ThingsBoard reenvía a n8n mediante webhook.
- Permite control remoto del LED integrado a través de atributos compartidos de ThingsBoard.

## Arquitectura

```
┌─────────────────────────────────────────────────────────────────────┐
│                         ESP32 (Firmware)                            │
│                                                                     │
│  ┌──────────────┐    ┌──────────────────────────────────────────┐   │
│  │  Botón GPIO4 │───>│              loop()                      │   │
│  │ (INPUT_PULLUP)│    │                                          │   │
│  └──────────────┘    │  1. Alarma (botón)                       │   │
│                      │     - Debounce 50ms                      │   │
│                      │     - Single-fire (evita spam)           │   │
│                      │     - Envía telemetría con alarm=1       │   │
│  ┌──────────────┐    │                                          │   │
│  │  LED GPIO2   │<───│  2. Telemetría periódica (cada 5s)      │   │
│  │  (OUTPUT)    │    │     - temp, humidity, uptime             │   │
│  └──────────────┘    │     - alarm=0 (no dispara webhook)      │   │
│                      └──────────────────────────────────────────┘   │
└────────────────────────────────┬────────────────────────────────────┘
                                 │ MQTT (puerto 1883)
                                 ▼
┌─────────────────────────────────────────────────────────────────────┐
│                     ThingsBoard IoT Platform                        │
│                                                                     │
│  ┌────────────────┐   ┌─────────────────┐   ┌──────────────────┐   │
│  │   Dispositivo  │──>│ Rule Chain       │──>│ Filtro alarm==1  │   │
│  │  (Telemetría)  │   │ (Procesamiento)  │   │ (Confirmar)      │   │
│  └────────────────┘   └─────────────────┘   └────────┬─────────┘   │
│                                                       │             │
│  ┌────────────────┐                                   │             │
│  │  Shared Attrs  │──> Control remoto LED             │             │
│  │  (led, command)│                                   │             │
│  └────────────────┘                                   │             │
└───────────────────────────────────────────────────────┼─────────────┘
                                                        │ Webhook HTTP
                                                        ▼
┌─────────────────────────────────────────────────────────────────────┐
│                      n8n (Workflow Automation)                       │
│                                                                     │
│  ┌──────────────┐   ┌─────────────────┐   ┌────────────────────┐   │
│  │   Webhook    │──>│  Procesamiento   │──>│  Acciones          │   │
│  │  (Trigger)   │   │  AI / Lógica     │   │  (Email, Slack,    │   │
│  └──────────────┘   └─────────────────┘   │   Telegram, etc.)  │   │
│                                            └────────────────────┘   │
└─────────────────────────────────────────────────────────────────────┘
```

### Flujo de datos

| Tipo | Origen | Datos | Destino | Dispara n8n |
|------|--------|-------|---------|-------------|
| **Alarma** | Botón (GPIO4) | `temp`, `alarm=1`, `status_msg`, `trigger`, `device_id` | ThingsBoard → n8n | Si |
| **Rutina** | Timer (5s) | `temp`, `humidity`, `alarm=0`, `uptime_min` | ThingsBoard | No |
| **Control** | ThingsBoard | Atributos compartidos (`led`, `command`) | ESP32 | No |

## Hardware

| Componente | Pin | Modo | Función |
|------------|-----|------|---------|
| Botón de alarma | GPIO 4 | `INPUT_PULLUP` | Dispara alerta manual |
| LED integrado | GPIO 2 | `OUTPUT` | Indicador / control remoto |

## Requisitos

- [PlatformIO CLI](https://platformio.org/install/cli) o [PlatformIO IDE](https://platformio.org/install/ide)
- Placa ESP32 DevKit
- Servidor ThingsBoard con dispositivo configurado
- Instancia n8n con webhook conectado a la Rule Chain de ThingsBoard

## Instalacion

1. Clonar el repositorio:
   ```bash
   git clone https://github.com/<tu-usuario>/n8n-thingsboard-AI.git
   cd n8n-thingsboard-AI
   ```

2. Configurar credenciales en `include/secrets.h`:
   ```cpp
   #define WIFI_NAME     "tu_red_wifi"
   #define WIFI_CONTRA   "tu_password"
   #define TG_SERVER     "tu_servidor_thingsboard"
   #define TG_TOKEN      "tu_token_de_dispositivo"
   ```

3. Compilar y subir al ESP32:
   ```bash
   pio run -e esp32dev --target upload
   ```

4. Monitorear la salida serial:
   ```bash
   pio device monitor
   ```

## Dependencias

| Libreria | Version | Uso |
|----------|---------|-----|
| [ThingsBoard](https://github.com/thingsboard/thingsboard-arduino-sdk) | 0.14.0 | Cliente MQTT para ThingsBoard |
| [TBPubSubClient](https://github.com/thingsboard/pubsubclient) | 2.10.0 | Transporte MQTT subyacente |
| [ArduinoHttpClient](https://github.com/arduino-libraries/ArduinoHttpClient) | ^0.6.1 | Utilidades HTTP |
| [ArduinoJson](https://github.com/bblanchon/ArduinoJson) | ^7.2.1 | Serialización JSON |

## Configuracion ThingsBoard

1. Crear un dispositivo y obtener el **Access Token**.
2. En la **Rule Chain** del dispositivo, agregar un nodo de filtro que verifique `alarm == 1`.
3. Conectar la salida del filtro a un nodo **REST API Call** apuntando al webhook de n8n.

## Configuracion n8n

1. Crear un workflow con un nodo **Webhook** como trigger.
2. Procesar el payload recibido (contiene `temp`, `status_msg`, `trigger`, `device_id`).
3. Agregar nodos de acción según se necesite (notificaciones por email, Slack, Telegram, etc.).
