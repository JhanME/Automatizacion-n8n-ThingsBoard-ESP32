#include <Arduino.h>

#if defined(ESP32)
#include <WiFi.h>
#endif

#include <Arduino_MQTT_Client.h>
#include <Shared_Attribute_Update.h>
#include <ThingsBoard.h>
#include "secrets.h"

// Configuración de pines
#define BUTTON_PIN 4
#define LED_BUILTIN 2

// Configuración de tiempos
#define DEBOUNCE_DELAY 50
constexpr int16_t TELEMETRY_SEND_INTERVAL = 2000;
constexpr uint32_t MAX_MESSAGE_SIZE = 256U;

// Atributos compartidos
constexpr const char LED_ATTR[] = "led";
constexpr const char COMMAND_ATTR[] = "command";

WiFiClient wifiClient;
Arduino_MQTT_Client mqttClient(wifiClient);
Shared_Attribute_Update<2U, 2U> shared_update;

const std::array<IAPI_Implementation*, 1U> apis = { &shared_update };

ThingsBoard tb(mqttClient, MAX_MESSAGE_SIZE, Default_Max_Stack_Size, apis);

int lastButtonState = HIGH;
int currentButtonState = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long lastTelemetrySend = 0;
bool ledState = false;
float temperature = 25.0;
float humidity = 60.0;
int uptime = 0;

void InitWiFi() {
  Serial.println("Conectando a WiFi...");
  WiFi.begin(WIFI_NAME, WIFI_CONTRA);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConectado");
}

const bool reconnect() {
  if (WiFi.status() == WL_CONNECTED) return true;
  InitWiFi();
  return true;
}

void processSharedAttributes(const JsonObjectConst &data) {
  for (auto it = data.begin(); it != data.end(); ++it) {
    if (strcmp(it->key().c_str(), LED_ATTR) == 0) {
      ledState = it->value().as<bool>();
      digitalWrite(LED_BUILTIN, ledState ? HIGH : LOW);
    }
  }
}

constexpr std::array<const char *, 2U> SHARED_ATTRIBUTES_LIST = { LED_ATTR, COMMAND_ATTR };
const Shared_Attribute_Callback<2U> attributes_callback(&processSharedAttributes, SHARED_ATTRIBUTES_LIST.cbegin(), SHARED_ATTRIBUTES_LIST.cend());

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
  InitWiFi();
  configTime(0, 0, "pool.ntp.org");
}

void loop() {
  if (!reconnect()) return;

  if (!tb.connected()) {
    if (!tb.connect(TG_SERVER, TG_TOKEN, 1883U)) {
      delay(5000);
      return;
    }
    shared_update.Shared_Attributes_Subscribe(attributes_callback);
  }

  int reading = digitalRead(BUTTON_PIN);
  if (reading != lastButtonState) lastDebounceTime = millis();

  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    if (reading != currentButtonState) {
      currentButtonState = reading;
      int buttonValue = (currentButtonState == LOW) ? 1 : 0;
      
      // SOLUCIÓN AL ERROR: Especificamos <2U> en la llamada
      std::array<Telemetry, 2U> button_packet = {
        Telemetry("button", (float)buttonValue),
        Telemetry("temperature", temperature)
      };
      tb.sendTelemetry<2U>(button_packet.cbegin(), button_packet.cend());

      if (buttonValue == 1) temperature = 35.0 + (random(0, 50) / 10.0);
    }
  }
  lastButtonState = reading;

  if (millis() - lastTelemetrySend >= TELEMETRY_SEND_INTERVAL) {
    lastTelemetrySend = millis();
    uptime++;

    if (temperature > 30.0) temperature -= 0.5;
    else temperature = 25.0 + (random(-20, 50) / 10.0);
    humidity = 60.0 + (random(-100, 100) / 10.0);

    int alarmStatus = (temperature > 30.0) ? 1 : 0;
    int buttonValue = (currentButtonState == LOW) ? 1 : 0;

    // SOLUCIÓN AL ERROR: Especificamos <5U> en la llamada
    std::array<Telemetry, 5U> t_data = {
      Telemetry("temperature", temperature),
      Telemetry("humidity",    humidity),
      Telemetry("button",      (float)buttonValue),
      Telemetry("uptime",      (float)uptime),
      Telemetry("alarm",       (float)alarmStatus)
    };

    Serial.println("Enviando paquete completo...");
    tb.sendTelemetry<5U>(t_data.cbegin(), t_data.cend());
  }

  tb.loop();
}