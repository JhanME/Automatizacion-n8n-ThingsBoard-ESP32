#include "wifi_manager.h"
#include "secrets.h"
#include <WiFi.h>
#include <Arduino.h>

void initWiFi() {
  Serial.print("Conectando a WiFi");
  WiFi.begin(WIFI_NAME, WIFI_CONTRA);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado — IP: " + WiFi.localIP().toString());
}

void checkWiFi() {
  if (WiFi.status() != WL_CONNECTED) initWiFi();
}