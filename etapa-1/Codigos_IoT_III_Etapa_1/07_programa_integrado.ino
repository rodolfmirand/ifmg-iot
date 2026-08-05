#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "secrets.h"

const int SENSOR_PIN = 34;
const int ACTUATOR_PIN = 2;

const char* DEVICE_ID = "aluno01";
const char* TOPIC_TELEMETRY = "ifmg/iot3/turmaA/aluno01/esp32/telemetry";
const char* TOPIC_COMMAND = "ifmg/iot3/turmaA/aluno01/esp32/command";
const char* TOPIC_STATE = "ifmg/iot3/turmaA/aluno01/esp32/state";
const char* TOPIC_AVAILABILITY = "ifmg/iot3/turmaA/aluno01/esp32/availability";

WiFiClient networkClient;
PubSubClient mqttClient(networkClient);

const unsigned long WIFI_RETRY_MS = 5000;
const unsigned long MQTT_RETRY_MS = 5000;
const unsigned long TELEMETRY_INTERVAL_MS = 5000;

unsigned long lastWifiAttempt = 0;
unsigned long lastMqttAttempt = 0;
unsigned long lastTelemetry = 0;
unsigned long sequenceNumber = 0;

void publishActuatorState(bool state, const char* requestId) {
  JsonDocument response;
  response["deviceId"] = DEVICE_ID;
  response["actuator"] = "led";
  response["state"] = state;
  response["requestId"] = requestId;

  char payload[192];
  size_t length = serializeJson(response, payload, sizeof(payload));
  mqttClient.publish(TOPIC_STATE, payload, length, true);
}

void onMqttMessage(char* topic, byte* payload, unsigned int length) {
  JsonDocument command;
  DeserializationError error = deserializeJson(command, payload, length);

  if (error) {
    Serial.print("JSON inválido: ");
    Serial.println(error.c_str());
    return;
  }

  const char* action = command["action"] | "";
  const char* target = command["target"] | "";
  const char* requestId = command["requestId"] | "sem-id";

  if (strcmp(action, "set") != 0 || strcmp(target, "led") != 0) {
    Serial.println("Comando não suportado.");
    return;
  }

  bool newState = command["value"] | false;
  digitalWrite(ACTUATOR_PIN, newState ? HIGH : LOW);
  publishActuatorState(newState, requestId);
}

void startWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  lastWifiAttempt = millis();
  Serial.println("Iniciando conexão Wi-Fi...");
}

void maintainWifi() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  unsigned long now = millis();
  if (now - lastWifiAttempt >= WIFI_RETRY_MS) {
    Serial.println("Tentando reconectar ao Wi-Fi...");
    WiFi.disconnect();
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    lastWifiAttempt = now;
  }
}

void connectMqtt() {
  if (WiFi.status() != WL_CONNECTED || mqttClient.connected()) {
    return;
  }

  unsigned long now = millis();
  if (now - lastMqttAttempt < MQTT_RETRY_MS) {
    return;
  }
  lastMqttAttempt = now;

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
    Serial.println("MQTT conectado.");
    mqttClient.publish(TOPIC_AVAILABILITY, "online", true);
    mqttClient.subscribe(TOPIC_COMMAND, 1);
    publishActuatorState(digitalRead(ACTUATOR_PIN) == HIGH, "startup");
  } else {
    Serial.print("Falha MQTT. Código: ");
    Serial.println(mqttClient.state());
  }
}

void publishTelemetry() {
  if (!mqttClient.connected()) {
    return;
  }

  unsigned long now = millis();
  if (now - lastTelemetry < TELEMETRY_INTERVAL_MS) {
    return;
  }
  lastTelemetry = now;

  JsonDocument telemetry;
  telemetry["deviceId"] = DEVICE_ID;
  telemetry["sensor"] = "analogico";
  telemetry["value"] = analogRead(SENSOR_PIN);
  telemetry["unit"] = "raw";
  telemetry["sequence"] = sequenceNumber++;
  telemetry["uptimeMs"] = now;
  telemetry["wifiRssi"] = WiFi.RSSI();

  char payload[256];
  size_t length = serializeJson(telemetry, payload, sizeof(payload));

  bool ok = mqttClient.publish(TOPIC_TELEMETRY, payload, length, false);
  Serial.print(ok ? "Publicado: " : "Falha ao publicar: ");
  Serial.println(payload);
}

void setup() {
  Serial.begin(115200);
  pinMode(ACTUATOR_PIN, OUTPUT);
  digitalWrite(ACTUATOR_PIN, LOW);

  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setCallback(onMqttMessage);
  mqttClient.setBufferSize(512);

  startWifi();
}

void loop() {
  maintainWifi();
  connectMqtt();

  if (mqttClient.connected()) {
    mqttClient.loop();
    publishTelemetry();
  }
}
