#include <ArduinoJson.h>

const int SENSOR_PIN = 34;
const char* TOPIC_TELEMETRY =
  "ifmg/iot3/turmaA/aluno01/esp32/telemetry";

unsigned long lastTelemetry = 0;
const unsigned long TELEMETRY_INTERVAL_MS = 5000;
unsigned long sequenceNumber = 0;

void publishTelemetry() {
  if (!mqttClient.connected()) {
    return;
  }

  unsigned long now = millis();
  if (now - lastTelemetry < TELEMETRY_INTERVAL_MS) {
    return;
  }
  lastTelemetry = now;

  int sensorValue = analogRead(SENSOR_PIN);

  JsonDocument document;
  document["deviceId"] = DEVICE_ID;
  document["sensor"] = "analogico";
  document["value"] = sensorValue;
  document["unit"] = "raw";
  document["sequence"] = sequenceNumber++;
  document["uptimeMs"] = now;

  char payload[256];
  size_t length = serializeJson(document, payload, sizeof(payload));

  bool published = mqttClient.publish(
    TOPIC_TELEMETRY,
    payload,
    length,
    false
  );

  Serial.print("Telemetria: ");
  Serial.print(payload);
  Serial.println(published ? " [enviada]" : " [falhou]");
}
