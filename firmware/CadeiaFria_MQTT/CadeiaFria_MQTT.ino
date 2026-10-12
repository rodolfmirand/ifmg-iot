// =====================================================================
// CadeiaFria_MQTT.ino — IoT III (IFMG Ouro Branco) — Entregas 3 a 5
//
// ESP32 + DS18B20 (GPIO19) + LED de alarme (GPIO4)
//  - Telemetria JSON a cada 15 s em .../telemetry  (QoS 0)
//  - Comandos em .../command  {"action":"set","target":"alarm|led","value":bool,"requestId":"..."}
//  - Confirmação em .../state (retido) SÓ depois de acionar o pino
//  - LWT "offline" em .../availability (retido); "online" ao conectar
//  - Wi-Fi/MQTT com reconexão NÃO bloqueante (sem delay no loop)
//
// Payloads compatíveis com o validador do gateway (validator.js).
// Bibliotecas: PubSubClient, ArduinoJson 7.x, OneWire, DallasTemperature
// =====================================================================
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "secrets.h"   // WIFI_SSID, WIFI_PASSWORD

// ----------------------------- Configuração -----------------------------
#define TURMA      "2026-1"
#define ALUNO      "rodolfo"
#define DEVICE_ID  "esp32cadeiafria"
#define TOPIC_BASE "ifmg/iot3/" TURMA "/" ALUNO "/" DEVICE_ID "/"

const char* TOPIC_TELEMETRY    = TOPIC_BASE "telemetry";
const char* TOPIC_COMMAND      = TOPIC_BASE "command";
const char* TOPIC_STATE        = TOPIC_BASE "state";
const char* TOPIC_AVAILABILITY = TOPIC_BASE "availability";

const char* MQTT_HOST = "broker.hivemq.com";
const uint16_t MQTT_PORT = 1883;

const uint8_t PIN_DS18B20 = 19;
const uint8_t PIN_LED     = 4;

const float TEMP_MIN_C = 2.0f;   // faixa segura da cadeia fria
const float TEMP_MAX_C = 8.0f;

const unsigned long TELEMETRY_INTERVAL_MS = 15000;
const unsigned long CONVERSION_MS         = 750;   // DS18B20 em 12 bits
const unsigned long WIFI_RETRY_MS         = 5000;
const unsigned long MQTT_RETRY_MS         = 5000;

// ------------------------------- Objetos -------------------------------
WiFiClient net;
PubSubClient mqtt(net);
OneWire oneWire(PIN_DS18B20);
DallasTemperature sensors(&oneWire);

char clientId[32];

// -------------------------------- Estado -------------------------------
bool armed       = true;    // alarme automático armado
bool ledOn       = false;   // estado físico do LED
bool ledManual   = false;   // true = LED controlado por comando "led" (ignora limiar)
bool tempValid   = false;
float lastTempC  = 0;
uint32_t sequence = 0;

unsigned long lastWifiTry = 0, lastMqttTry = 0, lastTelemetry = 0, convStart = 0;
bool converting = false;

// ------------------------------- Atuador -------------------------------
void setLed(bool on) {
  ledOn = on;
  digitalWrite(PIN_LED, on ? HIGH : LOW);
}

// Decide o LED pela regra da cadeia fria (exceto em modo manual)
void evaluateAlarm() {
  if (ledManual) return;
  bool outOfRange = tempValid && (lastTempC < TEMP_MIN_C || lastTempC > TEMP_MAX_C);
  setLed(armed && outOfRange);   // leitura inválida nunca acende o LED
}

// ------------------------------ Publicações ----------------------------
void publishState(const char* actuator, bool state, const char* requestId) {
  JsonDocument doc;
  doc["deviceId"] = DEVICE_ID;
  doc["actuator"] = actuator;            // "alarm" ou "led"
  doc["state"]    = state;
  if (requestId) doc["requestId"] = requestId;

  char buf[200];
  size_t n = serializeJson(doc, buf, sizeof(buf));
  bool ok = mqtt.publish(TOPIC_STATE, (const uint8_t*)buf, n, true);  // retido
  Serial.printf("[state] %s %s\n", ok ? "OK" : "FALHA", buf);
}

void publishTelemetry() {
  JsonDocument doc;
  doc["deviceId"] = DEVICE_ID;
  doc["sensor"]   = "temperatura";
  if (tempValid) doc["value"] = round(lastTempC * 100) / 100.0;  // 2 casas
  else           doc["value"] = nullptr;                           // falha de leitura
  doc["unit"]     = "C";
  doc["sequence"] = sequence++;
  doc["uptimeMs"] = millis();
  doc["wifiRssi"] = WiFi.RSSI();
  doc["armed"]    = armed;
  doc["ledOn"]    = ledOn;

  char buf[256];
  size_t n = serializeJson(doc, buf, sizeof(buf));
  bool ok = mqtt.publish(TOPIC_TELEMETRY, (const uint8_t*)buf, n, false);
  Serial.printf("[telemetry] %s %s\n", ok ? "OK" : "FALHA", buf);
}

// -------------------------------- Comandos -----------------------------
void onMessage(char* topic, byte* payload, unsigned int len) {
  JsonDocument cmd;
  DeserializationError err = deserializeJson(cmd, payload, len);
  if (err) { Serial.printf("[cmd] JSON inválido: %s\n", err.c_str()); return; }

  const char* action    = cmd["action"] | "";
  const char* target    = cmd["target"] | "";
  const char* requestId = cmd["requestId"] | "sem-id";

  if (strcmp(action, "set") != 0 || !cmd["value"].is<bool>()) {
    Serial.println("[cmd] ignorado: action deve ser \"set\" e value boolean");
    return;
  }
  bool value = cmd["value"].as<bool>();

  if (strcmp(target, "alarm") == 0) {
    armed = value;
    ledManual = false;           // volta ao modo automático
    evaluateAlarm();             // aplica no pino ANTES de confirmar
    publishState("alarm", armed, requestId);
  } else if (strcmp(target, "led") == 0) {
    ledManual = true;            // teste de bancada: força o LED
    setLed(value);
    publishState("led", ledOn, requestId);
  } else {
    Serial.printf("[cmd] target desconhecido: %s\n", target);
  }
}

// ------------------------- Wi-Fi / MQTT (não bloqueante) ----------------
void maintainWifi() {
  static bool printed = false;
  if (WiFi.status() == WL_CONNECTED) {
    if (!printed) { Serial.printf("[wifi] conectado IP=%s RSSI=%d\n", WiFi.localIP().toString().c_str(), WiFi.RSSI()); printed = true; }
    return;
  }
  printed = false;
  if (millis() - lastWifiTry >= WIFI_RETRY_MS) {
    lastWifiTry = millis();
    Serial.println("[wifi] tentando conectar...");
    WiFi.disconnect();
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  }
}

void maintainMqtt() {
  if (WiFi.status() != WL_CONNECTED) return;
  if (mqtt.connected()) { mqtt.loop(); return; }
  if (millis() - lastMqttTry < MQTT_RETRY_MS) return;
  lastMqttTry = millis();

  Serial.print("[mqtt] conectando... ");
  // connect(id, user, pass, willTopic, willQos, willRetain, willMessage)
  if (mqtt.connect(clientId, nullptr, nullptr, TOPIC_AVAILABILITY, 1, true, "offline")) {
    Serial.println("OK");
    mqtt.publish(TOPIC_AVAILABILITY, "online", true);
    mqtt.subscribe(TOPIC_COMMAND, 1);
    publishState("alarm", armed, "startup");
  } else {
    Serial.printf("falhou rc=%d\n", mqtt.state());
  }
}

// ---------------------- Sensor (máquina de estados) ---------------------
void maintainSensor() {
  unsigned long now = millis();
  if (!converting) {
    if (now - lastTelemetry >= TELEMETRY_INTERVAL_MS) {
      sensors.requestTemperatures();   // assíncrono (setWaitForConversion(false))
      convStart = now;
      converting = true;
    }
    return;
  }
  if (now - convStart < CONVERSION_MS) return;
  converting = false;
  lastTelemetry = now;

  float t = sensors.getTempCByIndex(0);
  tempValid = (t != DEVICE_DISCONNECTED_C && t > -55.0f && t < 125.0f);
  if (tempValid) lastTempC = t;
  else Serial.println("[sensor] falha de leitura do DS18B20");

  evaluateAlarm();                     // LED atualizado antes de publicar
  if (mqtt.connected()) publishTelemetry();
}

// ------------------------------ setup/loop -----------------------------
void setup() {
  Serial.begin(115200);
  pinMode(PIN_LED, OUTPUT);
  setLed(false);

  sensors.begin();
  sensors.setResolution(12);
  sensors.setWaitForConversion(false);
  Serial.printf("[sensor] DS18B20 encontrados: %d\n", sensors.getDeviceCount());

  uint64_t mac = ESP.getEfuseMac();
  snprintf(clientId, sizeof(clientId), "esp32cadeiafria-%04X", (uint16_t)(mac >> 32));

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  lastWifiTry = millis();

  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(onMessage);
  mqtt.setKeepAlive(30);
  mqtt.setBufferSize(512);
}

void loop() {
  maintainWifi();
  maintainMqtt();
  maintainSensor();
}
