#include <Arduino.h>
#include <WiFi.h>
#include <Arduino_MQTT_Client.h>
#include <Shared_Attribute_Update.h>
#include <ThingsBoard.h>

#include "secrets.h"
#include "config.h"
#include "network/wifi_manager.h"
#include "sensors/alarm.h"
#include "telemetry/telemetry.h"

// --- ThingsBoard ---
WiFiClient                      wifiClient;
Arduino_MQTT_Client             mqttClient(wifiClient);
Shared_Attribute_Update<2U, 2U> shared_update;

const std::array<IAPI_Implementation*, 1U> apis = { &shared_update };
ThingsBoard tb(mqttClient, MAX_MESSAGE_SIZE, Default_Max_Stack_Size, apis);

// --- Estado global ---
float         temperature      = 24.0;
int           uptime           = 0;
unsigned long lastTelemetrySend = 0;

// --- Shared attributes ---
void processSharedAttributes(const JsonObjectConst& data) {
  if (data.containsKey("led"))
    digitalWrite(LED_BUILTIN, data["led"].as<bool>() ? HIGH : LOW);
}

constexpr std::array<const char*, 2U> SHARED_ATTRS = { "led", "command" };
const Shared_Attribute_Callback<2U> attributes_callback(
  &processSharedAttributes,
  SHARED_ATTRS.cbegin(),
  SHARED_ATTRS.cend()
);

// ---

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
  initWiFi();
  configTime(-18000, 0, "pool.ntp.org");
}

void loop() {
  checkWiFi();

  if (!tb.connected()) {
    if (!tb.connect(TG_SERVER, TG_TOKEN, 1883U)) return;
    shared_update.Shared_Attributes_Subscribe(attributes_callback);
  }

  handleAlarmButton(tb, temperature);

  if (millis() - lastTelemetrySend >= TELEMETRY_SEND_INTERVAL) {
    lastTelemetrySend = millis();
    sendPeriodicTelemetry(tb, temperature, uptime);
  }

  tb.loop();
}