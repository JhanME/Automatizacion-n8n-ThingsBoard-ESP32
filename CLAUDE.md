# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

ESP32 Arduino firmware that connects to a ThingsBoard IoT platform via MQTT and triggers n8n automation workflows. The device reads a physical button (pin 4), sends alarm telemetry when pressed, and sends routine telemetry every 5 seconds. An LED (pin 2) can be controlled remotely via ThingsBoard shared attributes.

## Build & Upload Commands

This is a PlatformIO project targeting `esp32dev`.

```bash
# Build firmware
pio run -e esp32dev

# Upload to ESP32
pio run -e esp32dev --target upload

# Serial monitor (115200 baud)
pio device monitor

# Build, upload, and monitor in one step
pio run -e esp32dev --target upload && pio device monitor

# Clean build artifacts
pio run -e esp32dev --target clean
```

## Architecture

Single-file firmware (`src/main.cpp`) with two main execution paths in `loop()`:

1. **Alarm path** — Button press (debounced, single-fire) sends telemetry with `alarm=1` flag, which ThingsBoard forwards to n8n webhooks.
2. **Periodic path** — Every 5 seconds sends routine telemetry (`alarm=0`) with temperature, humidity, and uptime.

MQTT connection to ThingsBoard is managed via `ThingsBoard` library with `Arduino_MQTT_Client` transport. Shared attribute subscriptions allow remote LED control and command handling.

## Key Configuration

- **Hardware pins:** Button = GPIO 4 (INPUT_PULLUP), LED = GPIO 2 (OUTPUT)
- **Credentials:** WiFi and ThingsBoard server/token are in `include/secrets.h`
- **Timing:** Telemetry interval = 5000ms, debounce = 50ms
- **NTP timezone:** UTC-5 (offset -18000)
- **Max MQTT message size:** 512 bytes

## Dependencies (managed by PlatformIO)

- `thingsboard/ThingsBoard@0.14.0` — MQTT client for ThingsBoard
- `thingsboard/TBPubSubClient@2.10.0` — underlying MQTT pub/sub
- `arduino-libraries/ArduinoHttpClient@^0.6.1` — HTTP client
- `bblanchon/ArduinoJson@^7.2.1` — JSON serialization

## Important Notes

- `include/secrets.h` contains plaintext WiFi and ThingsBoard credentials — it is **not** gitignored. Be careful not to leak credentials.
- Temperature values are currently simulated with `random()`, not read from a real sensor.
- The test directory exists but has no tests yet. PlatformIO unit tests can be added there.
