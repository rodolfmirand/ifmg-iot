const int ACTUATOR_PIN = 2;
const char* TOPIC_COMMAND =
  "ifmg/iot3/turmaA/aluno01/esp32/command";
const char* TOPIC_STATE =
  "ifmg/iot3/turmaA/aluno01/esp32/state";

void publishActuatorState(bool state, const char* requestId) {
  JsonDocument response;
  response["deviceId"] = DEVICE_ID;
  response["actuator"] = "led";
  response["state"] = state;
  response["requestId"] = requestId;

  char payload[192];
  serializeJson(response, payload, sizeof(payload));
  mqttClient.publish(TOPIC_STATE, payload, true);
}

void onMqttMessage(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mensagem em ");
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

  if (strcmp(action, "set") != 0 || strcmp(target, "led") != 0) {
    Serial.println("Comando não suportado.");
    return;
  }

  bool newState = command["value"] | false;
  digitalWrite(ACTUATOR_PIN, newState ? HIGH : LOW);
  publishActuatorState(newState, requestId);
}

void subscribeToTopics() {
  bool subscribed = mqttClient.subscribe(TOPIC_COMMAND, 1);
  Serial.println(subscribed
    ? "Assinatura realizada."
    : "Falha ao assinar tópico.");
}
