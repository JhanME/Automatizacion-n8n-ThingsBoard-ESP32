#include "telemetry.h"
#include "config.h"
#include <Arduino.h>

void sendPeriodicTelemetry(ThingsBoard& tb, float& temperature, int& uptime) {
  uptime++;
  if (temperature > 25.0) temperature -= 2.0;

  // Ruido aleatorio ±0.5°C sobre la temp actual
  float tempNoise = temperature + (random(-5, 5) / 10.0f);

  // Humedad normal entre 55% y 70% con variación
  float humidity = 55.0f + (random(0, 150) / 10.0f);

  std::array<Telemetry, 7U> t_data = {
    Telemetry("deviceId",   DEVICE_ID),
    Telemetry("ts",         (float)millis()),
    Telemetry("temp",       tempNoise),
    Telemetry("humidity",   humidity),
    Telemetry("uptime_min", (float)uptime * (TELEMETRY_SEND_INTERVAL / 60000.0f)),
    Telemetry("severity",   "INFO"),
    Telemetry("alarm",      0.0f)
  };

  Serial.printf("Telemetría — Temp: %.2f°C | Humedad: %.1f%% | Uptime: %d\n",
                tempNoise, humidity, uptime);
  tb.sendTelemetry<7U>(t_data.cbegin(), t_data.cend());
}