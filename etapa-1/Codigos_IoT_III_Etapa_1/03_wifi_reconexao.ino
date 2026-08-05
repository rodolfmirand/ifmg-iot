#include <WiFi.h>
#include "secrets.h"

const unsigned long WIFI_RETRY_INTERVAL_MS = 5000;
unsigned long lastWifiAttempt = 0;

void printNetworkInformation() {
  Serial.print("IP local: ");
  Serial.println(WiFi.localIP());

  Serial.print("Gateway: ");
  Serial.println(WiFi.gatewayIP());

  Serial.print("Sinal (RSSI): ");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");
}

void startWifiConnection() {
  Serial.print("Conectando à rede ");
  Serial.println(WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  lastWifiAttempt = millis();
}

void maintainWifiConnection() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  unsigned long now = millis();
  if (now - lastWifiAttempt >= WIFI_RETRY_INTERVAL_MS) {
    Serial.println("Wi-Fi desconectado. Nova tentativa...");
    WiFi.disconnect();
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    lastWifiAttempt = now;
  }
}

void setup() {
  Serial.begin(115200);
  startWifiConnection();
}

void loop() {
  maintainWifiConnection();

  static bool informationPrinted = false;
  if (WiFi.status() == WL_CONNECTED && !informationPrinted) {
    Serial.println("Wi-Fi conectado.");
    printNetworkInformation();
    informationPrinted = true;
  }

  if (WiFi.status() != WL_CONNECTED) {
    informationPrinted = false;
  }
}
