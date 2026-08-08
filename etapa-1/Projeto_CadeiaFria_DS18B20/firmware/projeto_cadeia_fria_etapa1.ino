// =====================================================================
// projeto_cadeia_fria_etapa1.ino
//
// Internet das Coisas III — Etapa 1 (IFMG — Campus Ouro Branco)
// Projeto individual: monitoramento de cadeia fria (ex.: vacinas)
//
// Sensor:  DS18B20 à prova d'água (OneWire)
// Atuador: buzzer ativo (alarme sonoro local)
//
// Funcionalidades:
//  - Conexão Wi-Fi com reconexão automática não bloqueante.
//  - Conexão MQTT com Last Will (availability) e reconexão não bloqueante.
//  - Publicação periódica de telemetria (temperatura) em JSON.
//  - Assinatura do tópico de comando para armar/desarmar o alarme.
//  - Alarme automático por limiar de temperatura (TEMP_MIN_C / TEMP_MAX_C)
//    quando armado, além de controle remoto via MQTT.
//  - Publicação do estado confirmado do atuador (retained) sempre que
//    o buzzer muda de estado, seja por comando ou por limiar.
//  - Tratamento de JSON inválido e de comandos não suportados.
//
// Bibliotecas necessárias (Gerenciador de Bibliotecas da Arduino IDE):
//  - PubSubClient (Nick O'Leary)
//  - ArduinoJson (Benoit Blanchon) — versão 7.x (API JsonDocument)
//  - OneWire (Paul Stoffregen / Jim Studt)
//  - DallasTemperature (Miles Burton)
//
// Antes de compilar: preencha secrets.h e revise config.h
// (tópicos, pinos e limiares de temperatura).
// =====================================================================

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#include "secrets.h"
#include "config.h"

// ---------------------------------------------------------------------
// Objetos de rede, MQTT e sensor
// ---------------------------------------------------------------------
WiFiClient networkClient;
PubSubClient mqttClient(networkClient);

OneWire oneWire(ONE_WIRE_BUS_PIN);
DallasTemperature ds18b20(&oneWire);
DeviceAddress sensorAddress;
bool sensorFound = false;

// ---------------------------------------------------------------------
// Estado de temporização (tudo controlado por millis(), sem delay())
// ---------------------------------------------------------------------
unsigned long lastWifiAttempt = 0;
unsigned long lastMqttAttempt = 0;
unsigned long lastTelemetry = 0;
unsigned long sequenceNumber = 0;

// Máquina de estados simples para leitura não bloqueante do DS18B20:
// IDLE -> (dispara conversão) -> AGUARDANDO -> (tempo decorrido) -> IDLE
enum TempReadState { TEMP_IDLE, TEMP_AGUARDANDO_CONVERSAO };
TempReadState tempState = TEMP_IDLE;
unsigned long tempConversionStartedAt = 0;

// ---------------------------------------------------------------------
// Estado do domínio (cadeia fria)
// ---------------------------------------------------------------------
float lastTemperatureC = TEMP_ERROR_C;
bool temperatureValid = false;

bool alarmArmed = true;      // alarme começa armado por padrão (protege a carga)
bool buzzerOn = false;       // estado físico atual do buzzer

// ---------------------------------------------------------------------
// Utilitários de log
// ---------------------------------------------------------------------
void logLine(const char* prefix, const String& message) {
  Serial.print(prefix);
  Serial.println(message);
}

// =====================================================================
// Buzzer
// =====================================================================
void setBuzzer(bool on) {
  buzzerOn = on;
  bool level = BUZZER_ACTIVE_HIGH ? on : !on;
  digitalWrite(BUZZER_PIN, level ? HIGH : LOW);
}

// =====================================================================
// Wi-Fi
// =====================================================================
void printNetworkInformation() {
  Serial.print("IP local: ");
  Serial.println(WiFi.localIP());
  Serial.print("Gateway: ");
  Serial.println(WiFi.gatewayIP());
  Serial.print("Sinal (RSSI): ");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");
}

void startWifi() {
  Serial.print("Conectando à rede ");
  Serial.println(WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  lastWifiAttempt = millis();
}

void maintainWifi() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  unsigned long now = millis();
  if (now - lastWifiAttempt >= WIFI_RETRY_MS) {
    Serial.println("Wi-Fi desconectado. Nova tentativa...");
    WiFi.disconnect();
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    lastWifiAttempt = now;
  }
}

// =====================================================================
// MQTT — publicação de estado e telemetria
// =====================================================================
void publishActuatorState(bool state, const char* requestId) {
  JsonDocument response;
  response["deviceId"] = DEVICE_ID;
  response["actuator"] = "buzzer";
  response["state"] = state;
  response["armed"] = alarmArmed;
  response["requestId"] = requestId;

  char payload[224];
  size_t length = serializeJson(response, payload, sizeof(payload));

  bool published = mqttClient.publish(TOPIC_STATE, payload, length, true);
  Serial.print(published ? "Estado publicado: " : "Falha ao publicar estado: ");
  Serial.println(payload);
}

void publishTelemetry() {
  if (!mqttClient.connected() || !temperatureValid) {
    return;
  }

  JsonDocument telemetry;
  telemetry["deviceId"] = DEVICE_ID;
  telemetry["sensor"] = "temperatura";
  telemetry["value"] = lastTemperatureC;
  telemetry["unit"] = "C";
  telemetry["minSafeC"] = TEMP_MIN_C;
  telemetry["maxSafeC"] = TEMP_MAX_C;
  telemetry["alarmArmed"] = alarmArmed;
  telemetry["alarmActive"] = buzzerOn;
  telemetry["sequence"] = sequenceNumber++;
  telemetry["uptimeMs"] = millis();
  telemetry["wifiRssi"] = WiFi.RSSI();

  char payload[320];
  size_t length = serializeJson(telemetry, payload, sizeof(payload));

  bool ok = mqttClient.publish(TOPIC_TELEMETRY, payload, length, false);
  Serial.print(ok ? "Telemetria publicada: " : "Falha ao publicar telemetria: ");
  Serial.println(payload);
}

// =====================================================================
// MQTT — recepção e processamento de comando
// =====================================================================
void onMqttMessage(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mensagem recebida em ");
  Serial.println(topic);

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

  // Único alvo suportado nesta etapa: o alarme (buzzer) da cadeia fria.
  if (strcmp(action, "set") != 0 || strcmp(target, "alarm") != 0) {
    Serial.println("Comando não suportado (action/target inválidos).");
    return;
  }

  bool armRequested = command["value"] | false;
  alarmArmed = armRequested;
  Serial.print("Alarme ");
  Serial.println(alarmArmed ? "ARMADO por comando." : "DESARMADO por comando.");

  // Reavalia o estado físico do buzzer imediatamente com o novo modo.
  bool outOfRange = temperatureValid &&
      (lastTemperatureC < TEMP_MIN_C || lastTemperatureC > TEMP_MAX_C);
  bool shouldBuzz = alarmArmed && outOfRange;

  if (shouldBuzz != buzzerOn) {
    setBuzzer(shouldBuzz);
  }

  publishActuatorState(buzzerOn, requestId);
}

// =====================================================================
// MQTT — conexão com Last Will e reassinatura de tópicos
// =====================================================================
void connectMqtt() {
  if (WiFi.status() != WL_CONNECTED || mqttClient.connected()) {
    return;
  }

  unsigned long now = millis();
  if (now - lastMqttAttempt < MQTT_RETRY_MS) {
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
    mqttClient.subscribe(TOPIC_COMMAND, 1);
    Serial.println("Assinatura do tópico de comando realizada.");
    publishActuatorState(buzzerOn, "startup");
  } else {
    Serial.print(" falhou. Código de estado: ");
    Serial.println(mqttClient.state());
  }
}

void setupMqtt() {
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setCallback(onMqttMessage);
  mqttClient.setBufferSize(512);
}

// =====================================================================
// Sensor DS18B20 — leitura não bloqueante
// =====================================================================
void setupSensor() {
  ds18b20.begin();
  sensorFound = ds18b20.getAddress(sensorAddress, 0);

  if (sensorFound) {
    ds18b20.setResolution(sensorAddress, DS18B20_RESOLUTION_BITS);
    Serial.println("DS18B20 encontrado e configurado.");
  } else {
    Serial.println("ATENÇÃO: nenhum DS18B20 encontrado no barramento OneWire.");
  }

  // Conversão assíncrona: requestTemperatures() não bloqueia o loop.
  ds18b20.setWaitForConversion(false);
}

// Inicia uma nova conversão de temperatura, se estiver ocioso e o
// intervalo de telemetria estiver prestes a vencer.
void startTemperatureConversionIfDue() {
  if (tempState != TEMP_IDLE || !sensorFound) {
    return;
  }

  unsigned long now = millis();
  if (now - lastTelemetry < TELEMETRY_INTERVAL_MS) {
    return;
  }

  ds18b20.requestTemperatures();
  tempConversionStartedAt = now;
  tempState = TEMP_AGUARDANDO_CONVERSAO;
}

// Verifica se a conversão terminou, lê a temperatura e avalia o alarme.
void finishTemperatureConversionIfReady() {
  if (tempState != TEMP_AGUARDANDO_CONVERSAO) {
    return;
  }

  unsigned long now = millis();
  if (now - tempConversionStartedAt < DS18B20_CONVERSION_MS) {
    return;
  }

  float reading = ds18b20.getTempCByIndex(0);
  tempState = TEMP_IDLE;

  if (reading == DEVICE_DISCONNECTED_C || reading <= TEMP_ERROR_C + 1.0f) {
    Serial.println("Falha na leitura do DS18B20 (sensor desconectado?).");
    temperatureValid = false;
    return;
  }

  lastTemperatureC = reading;
  temperatureValid = true;

  Serial.print("Temperatura lida: ");
  Serial.print(lastTemperatureC);
  Serial.println(" C");

  evaluateAlarm();
  lastTelemetry = now;
  publishTelemetry();
}

// =====================================================================
// Lógica de alarme automático por limiar
// =====================================================================
void evaluateAlarm() {
  if (!temperatureValid) {
    return;
  }

  bool outOfRange = (lastTemperatureC < TEMP_MIN_C) || (lastTemperatureC > TEMP_MAX_C);
  bool shouldBuzz = alarmArmed && outOfRange;

  if (shouldBuzz != buzzerOn) {
    setBuzzer(shouldBuzz);
    Serial.println(shouldBuzz
      ? "ALARME: temperatura fora da faixa segura. Buzzer ligado."
      : "Temperatura normalizada ou alarme desarmado. Buzzer desligado.");

    if (mqttClient.connected()) {
      publishActuatorState(buzzerOn, "auto-limiar");
    }
  }
}

// =====================================================================
// setup / loop
// =====================================================================
void setup() {
  Serial.begin(115200);

  pinMode(BUZZER_PIN, OUTPUT);
  setBuzzer(false);

  setupSensor();
  setupMqtt();
  startWifi();
}

void loop() {
  maintainWifi();
  connectMqtt();

  if (mqttClient.connected()) {
    mqttClient.loop();
  }

  startTemperatureConversionIfDue();
  finishTemperatureConversionIfReady();

  static bool wifiInfoPrinted = false;
  if (WiFi.status() == WL_CONNECTED && !wifiInfoPrinted) {
    Serial.println("Wi-Fi conectado.");
    printNetworkInformation();
    wifiInfoPrinted = true;
  }
  if (WiFi.status() != WL_CONNECTED) {
    wifiInfoPrinted = false;
  }
}
