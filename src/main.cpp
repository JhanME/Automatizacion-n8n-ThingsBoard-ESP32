#include <Arduino.h>
#if defined(ESP32)
#include <WiFi.h>
#endif

#include <Arduino_MQTT_Client.h>
#include <Shared_Attribute_Update.h>
#include <ThingsBoard.h>
#include "secrets.h"

// --- Configuración ---
#define BUTTON_PIN 4
#define LED_BUILTIN 2
const char* DEVICE_ID = "AG-LIMA-001";
const char* LOCATION_VAL = "Laboratorio Central - UPCH";

#define DEBOUNCE_DELAY 50
constexpr int16_t TELEMETRY_SEND_INTERVAL = 5000;
constexpr uint32_t MAX_MESSAGE_SIZE = 1024U;

// Contador para generar alarmId único por sesión
unsigned long alarmCounter = 0;

WiFiClient wifiClient;
Arduino_MQTT_Client mqttClient(wifiClient);
Shared_Attribute_Update<2U, 2U> shared_update;
const std::array<IAPI_Implementation*, 1U> apis = { &shared_update };
ThingsBoard tb(mqttClient, MAX_MESSAGE_SIZE, Default_Max_Stack_Size, apis);

// --- Variables de Estado ---
int lastButtonState = HIGH;
int currentButtonState = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long lastTelemetrySend = 0;
float temperature = 24.0;
int uptime = 0;
bool alertaEnviada = false; 

void InitWiFi() {
  WiFi.begin(WIFI_NAME, WIFI_CONTRA);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
}

void processSharedAttributes(const JsonObjectConst &data) {
  if (data.containsKey("led")) digitalWrite(LED_BUILTIN, data["led"].as<bool>() ? HIGH : LOW);
}

constexpr std::array<const char *, 2U> SHARED_ATTRS = {"led", "command"};
const Shared_Attribute_Callback<2U> attributes_callback(&processSharedAttributes, SHARED_ATTRS.cbegin(), SHARED_ATTRS.cend());

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
  InitWiFi();
  configTime(-18000, 0, "pool.ntp.org");
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) InitWiFi();
  if (!tb.connected()) {
    if (!tb.connect(TG_SERVER, TG_TOKEN, 1883U)) return;
    shared_update.Shared_Attributes_Subscribe(attributes_callback);
  }

  // --- 1. LÓGICA DE ALARMA (Solo se envía UN mensaje a n8n) ---
  int reading = digitalRead(BUTTON_PIN);
  if (reading != lastButtonState) lastDebounceTime = millis();

  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    if (reading != currentButtonState) {
      currentButtonState = reading;
      if (currentButtonState == LOW && !alertaEnviada) {
        temperature = 38.5;
        alertaEnviada = true;
        alarmCounter++;

        // Generar alarmId único: DEVICE_ID-millis-contador
        char alarmId[64];
        snprintf(alarmId, sizeof(alarmId), "%s-%lu-%lu", DEVICE_ID, millis(), alarmCounter);

        unsigned long ts = millis(); // timestamp en ms desde boot (NTP se puede usar si se requiere epoch)

        Serial.println(">>> DISPARANDO WEBHOOK N8N (UNA SOLA VEZ) <<<");
        Serial.printf("alarmId: %s | severity: CRITICAL\n", alarmId);

        // Payload estándar: deviceId, ts, values (temp + humidity), severity, alarmId
        std::array<Telemetry, 7U> alarm_packet = {
          Telemetry("deviceId", DEVICE_ID),
          Telemetry("ts", (float)ts),
          Telemetry("temp", temperature),
          Telemetry("humidity", 60.0f + (random(-10, 10) / 10.0f)),
          Telemetry("severity", "CRITICAL"),
          Telemetry("alarmId", alarmId),
          Telemetry("alarm", 1.0f)  // Flag para el filtro de ThingsBoard Rule Chain
        };
        tb.sendTelemetry<7U>(alarm_packet.cbegin(), alarm_packet.cend());
      }
      else if (currentButtonState == HIGH) {
        alertaEnviada = false;
      }
    }
  }
  lastButtonState = reading;

  // --- 2. TELEMETRÍA PERIÓDICA (No dispara n8n) ---
  if (millis() - lastTelemetrySend >= TELEMETRY_SEND_INTERVAL) {
    lastTelemetrySend = millis();
    uptime++;

    if (temperature > 25.0) temperature -= 2.0;

    unsigned long ts = millis();

    // Payload estándar periódico: deviceId, ts, values (temp + humidity + uptime), severity INFO, sin alarmId
    std::array<Telemetry, 7U> t_data = {
      Telemetry("deviceId", DEVICE_ID),
      Telemetry("ts", (float)ts),
      Telemetry("temp", temperature),
      Telemetry("humidity", 60.0f + (random(-10, 10) / 10.0f)),
      Telemetry("uptime_min", (float)uptime * (TELEMETRY_SEND_INTERVAL / 60000.0f)),
      Telemetry("severity", "INFO"),
      Telemetry("alarm", 0.0f)  // No dispara n8n
    };

    Serial.printf("Monitor de Rutina - Temp: %.2f\n", temperature);
    tb.sendTelemetry<7U>(t_data.cbegin(), t_data.cend());
  }

  tb.loop();
}