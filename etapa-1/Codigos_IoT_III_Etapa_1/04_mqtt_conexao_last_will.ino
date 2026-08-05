#include <WiFi.h>
#include <PubSubClient.h>
#include "secrets.h"

WiFiClient networkClient;
PubSubClient mqttClient(networkClient);

const char* DEVICE_ID = "aluno01";
const char* TOPIC_AVAILABILITY =
  "ifmg/iot3/turmaA/aluno01/esp32/availability";

unsigned long lastMqttAttempt = 0;
const unsigned long MQTT_RETRY_INTERVAL_MS = 5000;

void connectMqtt() {
  if (WiFi.status() != WL_CONNECTED || mqttClient.connected()) {
    return;
  }

  unsigned long now = millis();
  if (now - lastMqttAttempt < MQTT_RETRY_INTERVAL_MS) {
    return;
  }
  lastMqttAttempt = now;

  Serial.print("Conectando ao broker MQTT...");

  bool connected = mqttClient.connect(
    DEVICE_ID,
    MQTT_USERNAME,
    MQTT_PASSWORD,
    TOPIC_AVAILABILITY,
    1,
    true,
    "offline"
  );

  if (connected) {
    Serial.println(" conectado.");
    mqttClient.publish(TOPIC_AVAILABILITY, "online", true);
  } else {
    Serial.print(" falhou. Estado: ");
    Serial.println(mqttClient.state());
  }
}

void setupMqtt() {
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setBufferSize(512);
}

void maintainMqtt() {
  connectMqtt();
  if (mqttClient.connected()) {
    mqttClient.loop();
  }
}
