#include "alarm.h"
#include "../config.h"
#include <Arduino.h>

static int           lastButtonState    = HIGH;
static int           currentButtonState = HIGH;
static unsigned long lastDebounceTime   = 0;
static bool          alertaEnviada      = false;
static unsigned long alarmCounter       = 0;

void handleAlarmButton(ThingsBoard& tb, float& temperature) {
  int reading = digitalRead(BUTTON_PIN);

  if (reading != lastButtonState) lastDebounceTime = millis();

  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    if (reading != currentButtonState) {
      currentButtonState = reading;

      if (currentButtonState == LOW && !alertaEnviada) {
        temperature = 36.0f + (random(0, 60) / 10.0f);
        alertaEnviada = true;
        alarmCounter++;

        char alarmId[64];
        snprintf(alarmId, sizeof(alarmId), "%s-%lu-%lu",
                 DEVICE_ID, millis(), alarmCounter);

        Serial.printf(">>> ALARMA | alarmId: %s | severity: CRITICAL\n", alarmId);

        std::array<Telemetry, 7U> alarm_packet = {
          Telemetry("deviceId",  DEVICE_ID),
          Telemetry("ts",        (float)millis()),
          Telemetry("temp",      temperature),
          Telemetry("humidity",  80.0f + (random(0, 150) / 10.0f)),
          Telemetry("severity",  "CRITICAL"),
          Telemetry("alarmId",   alarmId),
          Telemetry("alarm",     1.0f)
        };
        tb.sendTelemetry<7U>(alarm_packet.cbegin(), alarm_packet.cend());
      }
      else if (currentButtonState == HIGH) {
        alertaEnviada = false;
      }
    }
  }
  lastButtonState = reading;
}