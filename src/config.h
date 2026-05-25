#pragma once

#define BUTTON_PIN      4
#define LED_BUILTIN     2
#define DEBOUNCE_DELAY  50

constexpr int16_t  TELEMETRY_SEND_INTERVAL = 5000;
constexpr uint32_t MAX_MESSAGE_SIZE        = 1024U;

const char* const DEVICE_ID    = "AG-LIMA-001";
const char* const LOCATION_VAL = "Laboratorio Central - UPCH";